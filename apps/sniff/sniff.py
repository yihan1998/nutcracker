from scapy.all import *

# Define a callback function to process sniffed packets
def packet_callback(packet):
    print(packet.show())

def entrypoint():
    # Start sniffing (use iface="your_network_interface" to specify an interface)
    sniff(prn=packet_callback, count=10)