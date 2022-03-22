#!/usr/bin
#encoding: utf-8


import socket
import struct
import logging
import time

import docker
from lm_util import *

BUF_SIZE = 1024


class lm_socket:

    def __init__(self, dst_ip, port):
        HOST = dst_ip
        logging.info('destination ip is %s:' % HOST)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            self.socket.connect((HOST, port))
        except Exception as err:
            logging.error(
                'Error: There is something wrong in socket connect to the server node : %s.' % err)

    def send_file(self, file_path):
        file_handler = open(file_path, 'rb')
        self.socket.sendall(file_handler.read())
        file_handler.close()

    def send_msg(self, msg):
        logging.info('client send msg: %s ' % str(msg))
        msg_bytes = dict_tobytes(msg)
        self.socket.sendall(msg_bytes)

    def recv_msg(self):
        msg = bytes_todict(self.socket.recv(MAX_MSG_LENGTH))
        logging.info('client recv msg: %s' % str(msg))
        return msg

    def close(self):
        self.socket.close()


if __name__ == '__main__':
    pass
