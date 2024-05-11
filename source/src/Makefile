all: proto

proto: client_server.proto 
	protoc-c --c_out=. client_server.proto
	protoc --python_out=. client_server.proto

clean:
	@rm -f -v *.pb-c.* *_pb2.py *.o