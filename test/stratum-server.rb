##
# Simple simulation of a stratum server for Gapcoin
#
# needs gapcoind within your path, 
# needs Testnet daemon running
#
# For testing only!!!
#
#
require 'socket'
require 'json'

# Server bind to port 2000
server = TCPServer.new 2000 

CLIENTS = []

loop do                       
  Thread.start(server.accept) do |client|
    puts "[II] new client connected"
    CLIENTS << client

    Thread.start do 
      loop do
        line = client.gets
        next if line == nil || line.length < 3 
     
        obj = JSON.parse(line)
        puts obj.class
     
        puts "[II] received: #{ obj.inspect }" 
        
        # handles a getwork request
        if obj["method"] == "blockchain.block.request" then
          puts "[II] New Work Requested"

          msg =  "{ \"id\": #{ obj["id"] }, \"result\": { \"data\": "
          msg += "#{ `gapcoind --testnet getwork|grep data`.split(" : ").last.gsub(",", "").gsub("\n", "") },"   
          msg += "\"difficulty\": #{ `gapcoind --testnet getwork|grep difficulty`.split(" : ").last.gsub(",", "").gsub("\n", "") }"
          msg += " }, \"error\": null }"
     
          puts "[II] sending: #{ msg }"
          client.puts msg
        end
        
        # handles a submitted share
        if obj["method"] == "blockchain.block.submit"
          puts "[II] Share Submitted"

          msg =  "{ \"id\": #{ obj["id"] }, \"result\": #{ `gapcoind --testnet getwork #{ obj["params"]["data"] }`.gsub("\n", "") },"
          msg += "\"error\": null }"
     
          puts "[II] sending: #{ msg }"
          client.puts msg

          # new block? Notifies clients
          if msg.include? "true"
            msg =  "{ \"id\": null, \"method\": \"blockchain.block.new\", \"params\": "
            msg += "{ \"data\": #{ `gapcoind --testnet getwork|grep data`.split(" : ").last.gsub(",", "").gsub("\n", "") },"
            msg += "\"difficulty\": #{ `gapcoind --testnet getwork|grep difficulty`.split(" : ").last.gsub(",", "").gsub("\n", "") } } }"
         
            puts "[II] sending: #{ msg }"

            CLIENTS.each { |c| c.puts msg }
          end
        end
      end
    end
     
     # every 25 seconds informs clients about new blocks
     loop do
        sleep 25

        msg =  "{ \"id\": null, \"method\": \"blockchain.block.new\", \"params\": "
        msg += "{ \"data\": #{ `gapcoind --testnet getwork|grep data`.split(" : ").last.gsub(",", "").gsub("\n", "") },"
        msg += "\"difficulty\": #{ `gapcoind --testnet getwork|grep difficulty`.split(" : ").last.gsub(",", "").gsub("\n", "") } } }"

        puts "[II] sending: #{ msg }"
        client.puts msg
     end
  end
end
