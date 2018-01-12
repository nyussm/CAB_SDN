#!/usr/bin/env python
#
# based on the RunTimeEnvironment-remote example
# that comes generated by thrift
#
import argparse
import struct
import sys
import pprint
import collections
import time

sys.path.append('gen-py')

from urlparse import urlparse
from thrift.transport import TTransport
from thrift.transport import TZlibTransport
from thrift.transport import TSocket
#from thrift.transport import TSSLSocket
from thrift.transport import THttpClient
from thrift.protocol import TBinaryProtocol

from sdk6_rte import RunTimeEnvironment
from sdk6_rte.ttypes import *
    
if __name__ == '__main__':

    #
    # Argument handling
    #

    parser = argparse.ArgumentParser()

    parser.add_argument('-s','--server',
                        dest='rpc_server', default='localhost',
                        type=str,
                        help="Thrift RPC host (DEFAULT: localhost)")

    parser.add_argument('-p','--port',
                        dest='rpc_port', default='20206',
                        type=str,
                        help="Thrift RPC port (DEFAULT: 20206)")
    
    parser.set_defaults()

    args = parser.parse_args()
    
    use_zlib = 1
    
    host = args.rpc_server
    port = int(args.rpc_port)

    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    if use_zlib:
        transport = TZlibTransport.TZlibTransport(transport)

    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = RunTimeEnvironment.Client(protocol)
    transport.open()

    try:
        client.sys_ping()
    except TException:
        transport.close()
        sys.stderr.write("Basic communication with RPC server failed\n")
        sys.exit(1)

    # grab all the digest info
    digests = client.digest_list_all()

    # a map for associating registration handle with digest data
    digest_map = collections.OrderedDict()

    # register for each digest
    for d in digests:

        # get the digest registration handle
        dh = client.digest_register(d.id)
        if dh < 0:
            sys.stderr.write("Failed to register for digest %s\n" % d.name)
            sys.exit(1)

        # associate the registration handle with the digest data
        digest_map[dh] = {'desc' : d, 'count' : 0}

    print "polling for digests events"
    # okay now periodically retrieve and dump the digest data
    try:
        while 1:
            for dh, dgdata in digest_map.items():
                values = client.digest_retrieve(dh)

                if len(values) == 0: # no data
                    continue

                fldcnt = len(dgdata['desc'].fields)
                if len(values) % fldcnt != 0:
                    sys.stderr.write("Invalid field layout from digest %s" %
                                     dgdata['desc'].name)
                    sys.exit(1)
                
                for i in range(len(values) / fldcnt):
                    print "digest %s (P4 ID %d, P4 fieldlist %s)[%d] {" % (
                            dgdata['desc'].name,
                            dgdata['desc'].app_id,
                            dgdata['desc'].field_list_name,
                            dgdata['count'])

                    for flddesc, fielddata in zip(dgdata['desc'].fields, values[fldcnt * i:fldcnt * (i + 1)]):
                        print "    %s : %s" % (flddesc.name, fielddata)
                    print "}\n"

                    dgdata['count'] += 1

            time.sleep(2)
    except KeyboardInterrupt: # exit on control-c
        pass

    transport.close()
    
