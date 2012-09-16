require 'rubygems'
require 'mongo'

@conn = Mongo::Connection.new
@db   = @conn['pandorify']
@coll = @db['Person']

puts "There are #{@coll.count} records. Here they are:"
@coll.find.each { |doc| puts doc.inspect }