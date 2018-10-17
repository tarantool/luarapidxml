[![Build Status](https://travis-ci.org/tarantool/luarapidxml.svg?branch=master)](https://travis-ci.org/tarantool/luarapidxml)

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
