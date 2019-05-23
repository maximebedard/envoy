REQUEST = 0x80
HEADER = "CCnCCnNNQ"
OPCODES = {
  get: 0x00,
  set: 0x01,
  getq: 0x09,
  setq: 0x11,
}
OP_FORMAT = {
  get: 'a*',
  set: 'NNa*a*',
  setq: 'NNa*a*',
  getq: 'a*',
}
FORMAT = OP_FORMAT.inject({}) { |memo, (k, v)| memo[k] = HEADER + v; memo }

def get(key)
  [REQUEST, OPCODES[:get], key.bytesize, 0, 0, 0, key.bytesize, 0, 0, key].pack(FORMAT[:get])
end

def getq(key)
  [REQUEST, OPCODES[:getq], key.bytesize, 0, 0, 0, key.bytesize, 0, 0, key].pack(FORMAT[:getq])
end

def set(key, value, cas = 0, ttl = 0, flags = 0)
  [REQUEST, OPCODES[:set], key.bytesize, 8, 0, 0, value.bytesize + key.bytesize + 8, 0, cas, flags, ttl, key, value].pack(FORMAT[:set])
end

def setq(key, value, cas = 0, ttl = 0, flags = 0)
  [REQUEST, OPCODES[:setq], key.bytesize, 8, 0, 0, value.bytesize + key.bytesize + 8, 0, cas, flags, ttl, key, value].pack(FORMAT[:setq])
end

print(set("foo", "bar"))
# print(getq("bar"))
# 10.times { print(getq("foo")); print(getq("bar")) }

STDOUT.flush

# sleep(10)
# print(get("baz"))
