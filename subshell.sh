trap "kill 0" SIGINT
WS=$1
P=$2
Q=$3
export P
export Q
export WS
#(./mim 127.0.0.2 $P $Q)
#(./server $WS)
sh -c './mim 127.0.0.2 $P $Q'
sh -c './server $WS'
echo "Hi"
./client 127.0.0.1 $WS $P $Q 
