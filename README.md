[![pipeline status](https://gitlab.com/tarantool/ib-core/luarapidxml/badges/master/pipeline.svg)](https://gitlab.com/tarantool/ib-core/luarapidxml/commits/master)

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
