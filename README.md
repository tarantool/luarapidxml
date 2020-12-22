<a href="https://github.com/tarantool/luarapidxml/actions?query=workflow%3ATest">
<img src="https://github.com/tarantool/luarapidxml/workflows/Test/badge.svg">
</a>

# Fast XML parsing module for Tarantool

## Usage

```console
tarantool> xml = require('luarapidxml')
---
...

tarantool> xml.decode('<luarapidxml version="1.0.0"><rocks/></luarapidxml>')
---
- tag: luarapidxml
  attr:
    version: 1.0.0
  1:
    tag: rocks

tarantool> xml.encode({tag = 'input', attr = {type='text', name='password'}})
---
- <input type="text" name="password"/>
...


```
