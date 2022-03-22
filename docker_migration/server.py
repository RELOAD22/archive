#!/usr/bin/env python
# -*- coding:utf-8 -*-
#

import socket
import json
import socketserver
import logging
from lm_util import *
from lm_opts import *
import tarfile
from lm_container import *
import os
import os.path
workdir = ''


def get_size(path):
    sum = 0
    fileList = os.listdir(path)  # 获取path目录下所有文件
    for filename in fileList:
        pathTmp = os.path.join(path, filename)  # 获取path与filename组合后的路径
        if os.path.isdir(pathTmp):   # 判断是否为目录
            sum += get_size(pathTmp)        # 是目录就继续递归查找
        elif os.path.isfile(pathTmp):  # 判断是否为文件
            filesize = os.path.getsize(pathTmp)  # 如果是文件，则获取相应文件的大小
            sum += filesize      # 将文件的大小添加
    return sum


class ServerThread(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass


class lm_docker_server(socketserver.BaseRequestHandler):
    def recv_file(self, file_name, file_size):
        logging.info(
            'RECVING FILE: the size of files to recv : %d' % file_size)
        hd_file = open(file_name, 'wb')
        try:
            buffer = b''
            length = file_size
            while(length > 0):
                tmp = self.request.recv(length)
                if not tmp:
                    return False
                buffer = buffer + tmp
                length = file_size - len(buffer)

            hd_file.write(buffer)
            hd_file.close()
            logging.info('RECV FILE SUCCESS')
            return True
        except Exception as conError:
            logging.error('Error: connection error, conError:%s' % conError)

    def send_success(self):
        msg = {'taskid': self.taskid, 'status': 'success'}
        self.send_msg(msg)

    def send_msg(self, msg):
        logging.info('server send msg: %s ' % str(msg))
        msg_bytes = dict_tobytes(msg)
        self.request.sendall(msg_bytes)

    def recv_msg(self):
        msg = bytes_todict(self.request.recv(MAX_MSG_LENGTH))
        logging.info('server recv msg: %s' % str(msg))
        return msg

    def handle_request(self, request):
        command_type = request["type"]
        if command_type == 'check':
            pass
        if command_type == 'init':
            global workdir
            self.taskid = request["taskid"]
            self.workpath = workdir + '/'+self.taskid
            logging.info('work path: %s' % self.workpath)
            isExists = os.path.exists(self.workpath)
            if not isExists:
                os.makedirs(self.workpath)

        if command_type == 'files':
            self.tar_name = request['file_name']
            self.tar_size = request['file_size']
            if request['sendType'] == 'socket':
                self.send_success()
                self.recv_file(self.workpath + '/' +
                               self.tar_name, self.tar_size)
            else:
                msg = {'taskid': self.taskid, 'status': 'success',
                       'sendPath': self.workpath}
                self.send_msg(msg)
                return True

        if command_type == 'restore':
            tar_file = tarfile.open(self.workpath + '/' + self.tar_name, 'r')
            tar_file.extractall(self.workpath)
            tar_file.close()
            if False:
                # 从本地文件中读出镜像
                dump_sh = 'docker load -i ' + self.workpath + '/' + self.tarnewimage_name
                exec_shell(dump_sh, self.taskid)

            container_name = request['container_name']
            self.container_id = dockerclient.get_container_id(container_name)
            if self.container_id:
                container_name = container_name + '_' + self.taskid
            restore_arguments = request['arguments']
            newimage_name = request['newimage_name']
            restore_cmd = request['Cmd']

            dump_sh = 'docker create --name '\
                + container_name\
                + restore_arguments\
                + ' ' + newimage_name\
                + restore_cmd
            exec_shell(dump_sh, self.taskid)

            self.container_id = dockerclient.get_container_id(container_name)
            if not self.container_id:
                logging.error('Error: get docker container id failed')
                return False
            '''
			dump_sh = 'docker start '+ container_name
			exec_shell(dump_sh)
			
			dump_sh = 'docker stop '+ container_name
			exec_shell(dump_sh)'''

            diffsize = get_size(self.workpath+'/'+'diff/')
            logging.debug("%s: diffsize: %dBtyes" % (self.taskid, diffsize))
            dumpsize = get_size(self.workpath+'/'+'dumpimages/')
            logging.debug("%s: dumpsize: %dBtyes" % (self.taskid, dumpsize))

            if True:
                Upperdir = dockerclient.get_container_UpperDir(container_name)
                dump_sh = 'cp -r ' + self.workpath+'/'+'diff/' +\
                    ' ' + Upperdir[0:-4]
                if not exec_shell(dump_sh, self.taskid):
                    return False

            dump_sh = 'cp -r ' + self.workpath+'/'+'dumpimages/' +\
                ' ' + '/var/lib/docker/containers/'\
                + self.container_id + '/checkpoints/'
            if not exec_shell(dump_sh, self.taskid):
                return False

            dump_sh = 'docker start --checkpoint=' + 'dumpimages/' +\
                ' ' + container_name
            if not exec_shell(dump_sh, self.taskid):
                return False

        if command_type == 'close':
            return False

        self.send_success()

        return True

    def handle(self):
        while True:
            request = self.recv_msg()
            if self.handle_request(request):
                continue
            else:
                break

        logging.info('handle finish taskid:%s' % self.taskid)


def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    return socket.inet_ntoa(fcntl.ioctl(s.fileno(), 0x8915,
                                        socket.pack('256s', ifname[:15]))[20:24])


def get_host_ip():
    """
    查询本机ip地址
    :return: ip
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()

    return ip


def lm_service():
    global workdir
    workdir = os.getcwd()
    host = get_host_ip()
    #host = socket.gethostname()
    print(host)
    HOST = host
    server = ServerThread((HOST, PORT), lm_docker_server)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logging.debug('server node stop because of keyboard interrupt')
        server.shutdown()
    server.server_close()
