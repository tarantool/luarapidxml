#!/usr/bin/env tarantool

require('strict').on()
local tap = require('tap')
local fio = require('fio')
local luarapidxml = require('luarapidxml')
local encode = luarapidxml.encode
local decode = luarapidxml.decode

local nestedtag_txt =
    [[<nested_out key1="val1" key2="val2">]]..
        [[<inside_1>some text</inside_1>]]..
        [[<inside_2>more text</inside_2>]]..
        [[<inside_3>]]..
            [[3.1]]..
            [[<3.2 foo="bar"/>]]..
            [[3.3]]..
        [[</inside_3>]]..
        [[<escapes>]]..
            [[<lt ch="&lt;">&lt;-</lt>]]..
            [[<gt ch="&gt;">-&gt;</gt>]]..
            [[<amp ch="&amp;">+&amp;+</amp>]]..
            [[<quot ch="&quot;">&quot;</quot>]]..
            [[<apos ch="&apos;">&apos;</apos>]]..
            [[<mix>&apos;&quot;&amp;&lt;&gt;&amp;&quot;&apos;</mix>]]..
        [[</escapes>]]..
        [[your ad could be here]]..
    [[</nested_out>]]
local nestedtag_lom = {
    tag = "nested_out",
    attr = {
        key1 = "val1",
        key2 = "val2",
    },
    [1] = {
        tag = "inside_1",
        [1] = "some text",
    },
    [2] = {
        tag = "inside_2",
        [1] = "more text",
    },
    [3] = {
        tag = "inside_3",
        [1] = "3.1",
        [2] = {tag = "3.2", attr = {foo = "bar"}},
        [3] = "3.3",
    },
    [4] = {
        tag = "escapes",
        {tag = "lt",   attr={ch=[[<]]}, '<-'},
        {tag = "gt",   attr={ch=[[>]]}, '->'},
        {tag = "amp",  attr={ch=[[&]]}, '+&+'},
        {tag = "quot", attr={ch=[["]]}, '"'},
        {tag = "apos", attr={ch=[[']]}, "'"},
        {tag = "mix", [['"&<>&"']]},
    },
    [5] = "your ad could be here",
}

local test = tap.test("luarapidxml")
test:plan(15)

---------------------------------
test:diag("Test decoding errors")

test:is_deeply(
    {pcall(decode, "")},
    {true, nil, "decode element: not a xml element"},
    "decode empty string"
)
test:is_deeply(
    {pcall(decode, "<>")},
    {true, nil, "invalid xml string: expected element name"},
    "decode invalid xml string"
)
test:is_deeply(
    {pcall(decode, '<x ch="&xxx;"/>')},
    {true, nil, "xml decode: invalid escape sequence"},
    "decode invalid escape sequence"
)
test:is_deeply(
    {pcall(decode, '<x ch="&#x0x201234;"/>')},
    {true, nil, "xml decode: invalid unicode codepoint"},
    "decode invalid unicode codepoint"
)

---------------------------------
test:diag("Test encoding errors")

test:is_deeply(
    {pcall(encode, {})},
    {true, nil, "encode element: Invalid table format" ..
        " (`tag' field must be a string)"},
    "encode invalid tag (nil)"
)
test:is_deeply(
    {pcall(encode, {tag = 0})},
    {true, nil, "encode element: Invalid table format" ..
        " (`tag' field must be a string)"},
    "encode invalid tag (number)"
)
test:is_deeply(
    {pcall(encode, {tag = 'x', attr = 'y'})},
    {true, nil, "encode element: Invalid table format" ..
        " (`attr' field must be a table)"},
    "encode invalid attr (string)"
)
test:is_deeply(
    {pcall(encode, {tag = 'x', attr = {'y'}})},
    {true, nil, "encode element: Invalid table format" ..
        " (`attr' table must have string keys and values)"},
    "encode invalid attr (numeric key)"
)
test:is_deeply(
    {pcall(encode, {tag = 'x', attr = {y = 0}})},
    {true, nil, "encode element: Invalid table format" ..
        " (`attr' table must have string keys and values)"},
    "encode invalid attr (numeric value)"
)
test:is_deeply(
    {pcall(encode, {tag = 'x', 0ULL})},
    {true, nil, "encode element: Invalid table format" ..
        " (unknown content type)"},
    "encode invalid conent (cdata)"
)

------------------------------------------
test:diag("Test unicode escape sequences")

local escape_dec = "&#37; &#169; &#339; &#2063; &#23449; &#128526;"
local escape_hex = "&#x25; &#xA9; &#x153; &#x80F; &#x5B99; &#x1F60E;"
local escape_utf = "% © œ ࠏ 宙 😎"
test:is_deeply(
    decode("<utf>"..escape_dec.."</utf>"),
    {tag = "utf", escape_utf},
    "decode 'dec escapes'"
)
test:is_deeply(
    decode("<utf>"..escape_hex.."</utf>"),
    {tag = "utf", escape_utf},
    "decode 'hex escapes'"
)
test:is_deeply(
    decode(nestedtag_txt),
    nestedtag_lom,
    "decode 'nestedtag'"
)
test:is(
    encode(nestedtag_lom),
    nestedtag_txt,
    "encode 'nestedtag'"
)

-----------------------------------------
test:diag("Test transcoding performance")

local function read_file(path)
    local file = fio.open(path)
    if file == nil then
        local err = string.format('Failed to open file %s', path)
        return error(err)
    end
    local buf = {}
    while true do
        local val = file:read(1024)
        if val == nil then
            local err = string.format('Failed to to read from file %s', path)
            return error(err)
        elseif val == '' then
            break
        end
        table.insert(buf, val)
    end
    file:close()
    return table.concat(buf, '')
end

test:test("fixtures", function(test)
    local fixtures_names = {"ebay", "reed", "customer"}
    test:plan(#fixtures_names)
    local dec_band_num = 0
    local dec_band_den = 0
    local enc_band_num = 0
    local enc_band_den = 0
    for _, name in ipairs(fixtures_names) do

        local content_txt = read_file('./fixtures/'..name..'.xml')
        local content_lom = decode(content_txt)

        test:diag("FIXTURE '"..name.."'")
        test:diag(string.format("size: %.2f KiB", #content_txt/1024.0 ))

        local start = os.clock()
        local stop
        local cnt = 0
        repeat
            stop = os.clock()
            cnt = cnt+1
            decode(content_txt)
        until stop - start > 3

        test:diag(string.format("decode: %.2f Req/s", cnt/(stop-start) ))
        dec_band_num = dec_band_num + (#content_txt*cnt)
        dec_band_den = dec_band_den + (stop-start)

        local start = os.clock()
        local stop
        local cnt = 0
        repeat
            stop = os.clock()
            cnt = cnt+1
            encode(content_lom)
        until stop - start > 3

        test:diag(string.format("encode: %.2f Req/s", cnt/(stop-start) ))
        enc_band_num = enc_band_num + (#content_txt*cnt)
        enc_band_den = enc_band_den + (stop-start)

        -- we can not compare encoded xml strings
        -- because xml attribute order is not determined
        -- thus we compare decoded lua tables recursively
        test:is_deeply(
            decode(encode(content_lom)),
            content_lom,
            "reencode '"..name.."'"
        )
    end
    test:diag(string.format("Bandwidth (decode average): %.2f MiB/s", dec_band_num/dec_band_den/1024/1024))
    test:diag(string.format("Bandwidth (encode average): %.2f MiB/s", enc_band_num/enc_band_den/1024/1024))

end)

os.exit(test:check())
