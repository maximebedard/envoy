require "redis"
require "pry-byebug"

r = Redis.new(port: 1999)
r.set("foo", 1)
r.set("bar", 2)
1000000.times do
  result = r.pipelined do
    r.get("foo")
    r.get("bar")
  end
  puts "out of order detected" if result != ["1", "2"]
end
