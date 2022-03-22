#!/usr/bin
#encoding: utf-8

import socket
import struct
import logging
import time
from lm_container import dockerclient
from lm_util import *
import lm_client
import subprocess as sp
import tarfile
import os
import os.path

files = {}


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


class File:
    def __init__(self):
        self.name = ''
        self.path = ''

    def getsize(self):
        return os.path.getsize(self.path)


class MigrateFiles:
    def __init__(self, taskid, workdir):
        self.taskid = taskid
        self.workpath = workdir + '/' + taskid
        logging.info('%s work path: %s' % (taskid, self.workpath))
        isExists = os.path.exists(self.workpath)
        if not isExists:
            os.makedirs(self.workpath)

        self.dumpimages = File()
        self.dumpimages.name = 'dumpimages'
        self.dumpimages.path = self.workpath + '/' + self.dumpimages.name

        self.tardockerimage = File()
        self.tardockerimage.name = 'dockerimage.tar'
        self.tardockerimage.path = self.workpath + '/' + self.tardockerimage.name

        self.tarname = self.taskid + '.tar'
        self.tarpath = self.workpath + '/' + self.tarname


def tardir(srcdir, dstpath):

    tar = tarfile.open(dstpath, 'w')
    if os.path.isdir(srcdir) == False:
        logging.error("ERROR: tar's target is not dir")
        return False
    tar.add(srcdir, os.path.basename(srcdir))
    tar.close()
    tar_size = os.path.getsize(dstpath)
    return tar_size


class live_migrate:
    def __init__(self, migrate_opts):
        self.migrate_opts = migrate_opts
        self.lmtask_id = random_str()

        self.container_name = migrate_opts['name']
        self.container_id = dockerclient.get_container_id(self.container_name)

        self.dst_ip = migrate_opts['destip']
        self.port = migrate_opts['port']

        self.workdir_path = os.getcwd()

        self.files = MigrateFiles(self.lmtask_id, self.workdir_path)

        self.tar = tarfile.open(self.files.tarpath, 'w')
        settime(self.lmtask_id)
        # 在目的主机上要根据这个image创建新的容器
        if migrate_opts['dockerfileModes'] == 'layer':
            self.new_imagename = dockerclient.get_container_image(
                self.container_name)
        elif migrate_opts['dockerfileModes'] == 'images':
            self.new_imagename = self.container_name + '_' + self.lmtask_id

        logging.info(
            "-----------------Live Migrate begin ---------------------")
        logging.info("Live Migrate task id: %s" % self.lmtask_id)
        logging.info("Live Migrate Container name: %s id: %s taskid: %s" % (
            self.container_name, self.container_id, self.lmtask_id))
        logging.info("Live Migrate destination's ip: %s " % self.dst_ip)

    def socket_build(self):
        '''socket build and check'''
        lm_socket = lm_client.lm_socket(self.dst_ip, self.port)
        msg = {'taskid': self.lmtask_id, 'type': 'init'}
        lm_socket.send_msg(msg)

        data = lm_socket.recv_msg()
        if data['status'] != 'success':
            logging.error('socket build failed')
            return False

        logging.info('%s: Socket connect to ip: %s success' %
                     (self.lmtask_id, self.dst_ip))
        return lm_socket

    def dump_container(self):
        '''dump the container's process to images'''
        '''
		dump_sh = 'docker checkpoint create --checkpoint-dir=' \
					+ self.files.workpath +\
			  		' ' + self.container_name \
					+ ' ' + self.files.dumpimages.name'''

        isExists = os.path.exists(
            '/var/run/docker/runtime-runc/moby/'+self.container_id)
        if not isExists:
            return False

        dump_sh = 'runc --root /var/run/docker/runtime-runc/moby/' +\
            ' --debug --log ' + self.files.workpath + '/runc.log' +\
            ' checkpoint ' + \
            ' --work-path ' + self.files.workpath +\
            ' --image-path ' + self.files.dumpimages.path +\
            ' --tcp-established --ext-unix-sk --file-locks --shell-job --auto-dedup' + \
            ' ' + self.container_id
        if not exec_shell(dump_sh, self.lmtask_id):
            return False
        return True

    def dump_dockerimage(self):
        '''DOCKER commit and save container to image'''
        dump_sh = 'docker commit ' + self.container_name +\
            ' ' + self.new_imagename
        exec_shell(dump_sh, self.lmtask_id)

        dump_sh = 'docker save -o ' + self.files.tardockerimage.path + \
            ' ' + self.new_imagename
        exec_shell(dump_sh, self.lmtask_id)
        self.tar.add(self.files.tardockerimage.path, self.new_imagename)
        return True

    def dump_diff(self):
        '''tar rw layer of container'''
        UpperDir = dockerclient.get_container_UpperDir(self.container_name)
        #logging.debug("%s: tar diff"%self.lmtask_id)
        self.tar.add(UpperDir, 'diff')
        diffsize = get_size(UpperDir)
        logging.debug("%s: diff: %dBytes" % (self.lmtask_id, diffsize))
        #logging.debug("%s: tar finish"%self.lmtask_id)
        return True

    def dump_volumes(self):
        '''tar vols of container(in new name)'''
        #logging.debug("%s: tar volumes"%self.lmtask_id)
        for count in range(len(self.mountinfo)):
            if 'Newname' not in self.mountinfo[count]:
                # 有新名字的卷才需要转储
                continue
            tarvol_name = self.mountinfo[count]['Newname']

            Source = self.mountinfo[count]['Source']
            if os.path.isdir(Source) == False:
                logging.error("ERROR: tar's target is not dir")
                return False
            self.tar.add(Source, tarvol_name)

            count += 1
        #logging.debug("%s: tar finish"%self.lmtask_id)
        return True

    def restore(self, container_name, restore_arguments_str, newimage_name, restore_cmd_str):
        '''send retore msg to dest ip'''
        msg = {'taskid': self.lmtask_id, 'type': 'restore',
               'container_name': container_name,
               'arguments': restore_arguments_str,
               'newimage_name': newimage_name,
               'Cmd': restore_cmd_str
               }
        self.lm_socket.send_msg(msg)
        data = self.lm_socket.recv_msg()
        if data['status'] != 'success':
            logging.error('dst node restore container:%s from image:%s failed' % (
                container_name, newimage_name))
            return False
        return True

    def send_file(self, filepath, filetype):
        '''send file to destip(docket or scp)'''
        filename = os.path.basename(filepath)
        filesize = os.path.getsize(filepath)
        if self.migrate_opts['sendType'] == 'socket':
            msg = {'taskid': self.lmtask_id, 'type': filetype,
                   'sendType': 'socket', 'file_size': filesize, 'file_name': filename}
        else:
            msg = {'taskid': self.lmtask_id, 'type': filetype,
                   'sendType': 'scp', 'file_size': filesize, 'file_name': filename}

        self.lm_socket.send_msg(msg)
        data = self.lm_socket.recv_msg()
        if data['status'] != 'success':
            logging.error('send files msg failed')
            return False
        if self.migrate_opts['sendType'] == 'socket':
            self.lm_socket.send_file(filepath)
            data = self.lm_socket.recv_msg()
            if data['status'] != 'success':
                logging.error('send files failed')
                return False
        else:
            dump_sh = 'scp ' + filepath +\
                ' ' + 'root@' + self.dst_ip + ':' +\
                data['sendPath'] + '/'
            exec_shell(dump_sh, self.lmtask_id)
        return True

    def close(self):
        '''send close msg to destip'''
        msg = {'taskid': self.lmtask_id, 'type': 'close'}
        self.lm_socket.send_msg(msg)
        '''
		data = self.lm_socket.recv_msg()
		if data['status'] != 'success':
			logging.error('restore failed')
			return False'''

        self.lm_socket.close()
        return True

    def getlevel(self, dumporder):
        for level in range(len(dumporder)):
            for c in dumporder[level]:
                if c == self.container_name:
                    return level

    def check_beforedump(self, dumporder):
        if not dumporder:
            return True
        if self.getlevel(dumporder) > 0:
            # 循环直到上一等级所有容器进程被转储
            flag = True
            while flag:
                for c1 in dumporder[self.getlevel(dumporder) - 1]:
                    if getstate_ofcontainer(c1) < TARED:
                        flag = True
                        time.sleep(0.1)
                        break
                    else:
                        flag = False
            '''			
			flag = True
			while flag:	
				for c1 in dumporder[self.getlevel(dumporder)]:
					if c1 == self.container_name:
						flag = False
						break
					if getstate_ofcontainer(c1) < DUMPED:
						flag = True
						time.sleep(0.1)
						break
					else :
						flag = False'''

    def check_beforetar(self, restoreorder):
        if not restoreorder:
            return True
        if self.getlevel(restoreorder) > 0:
            # 循环直到上一等级所有容器进程被恢复
            flag = True
            while flag:
                for c1 in restoreorder[self.getlevel(restoreorder) - 1]:
                    if getstate_ofcontainer(c1) < TARED:
                        flag = True
                        time.sleep(0.1)
                        break
                    else:
                        flag = False
            '''
			flag = True
			while flag:	
				for c1 in restoreorder[self.getlevel(restoreorder)]:
					if c1 == self.container_name:
						flag = False
						break
					if getstate_ofcontainer(c1) < TARED:
						flag = True
						time.sleep(0.1)
						break
					else :
						flag = False'''

    def check_beforesend(self, restoreorder):
        if not restoreorder:
            return True
        if self.getlevel(restoreorder) > 0:
            # 循环直到上一等级所有容器进程被恢复
            flag = True
            while flag:
                for c1 in restoreorder[self.getlevel(restoreorder) - 1]:
                    if getstate_ofcontainer(c1) < SENDED:
                        flag = True
                        time.sleep(0.1)
                        break
                    else:
                        flag = False

    def check_beforerestore(self, restoreorder):
        if not restoreorder:
            return True
        if self.getlevel(restoreorder) > 0:
            # 循环直到上一等级所有容器进程被恢复
            flag = True
            while flag:
                for c1 in restoreorder[self.getlevel(restoreorder) - 1]:
                    if getstate_ofcontainer(c1) < RESTORED:
                        flag = True
                        time.sleep(0.1)
                        break
                    else:
                        flag = False
            '''
			flag = True
			while flag:	
				for c1 in restoreorder[self.getlevel(restoreorder)]:
					if c1 == self.container_name:
						flag = False
						break
					if getstate_ofcontainer(c1) < RESTORED:
						flag = True
						time.sleep(0.1)
						break
					else :
						flag = False		'''

    def run(self, dumporder, restoreorder):
        #----check container status: before migrate, we must ensure the container is running in host.----#
        if not dockerclient.check_container_running(self.container_id):
            logging.error(
                'Error: Container which you want to migrate is not running.')
            return False

        #----build socket with dst ip and check the dst status ----#
        self.lm_socket = self.socket_build()
        if not self.lm_socket:
            return False

        # ----------------------------------DUMP------------
        self.check_beforedump(dumporder)
        #---dump the container's process to images---#
        settime(self.lmtask_id)
        if not self.dump_container():
            return False
        settime(self.lmtask_id)
        setstate_ofcontainer(self.container_name, DUMPED)

        # ----------------------------------TAR------------
        # if self.migrate_opts['migratemode'] == 'pipeline':
        #	self.check_beforetar(restoreorder)
        settime(self.lmtask_id)
        restore_arguments_str, restore_cmd_str, self.mountinfo = dockerclient.set_arguments_fromconfig(
            self.migrate_opts, self.lmtask_id)
        #restore_arguments_str += ' ' + self.migrate_opts['debug'] + ' '
        logging.debug("%s: tar dumpimages" % self.lmtask_id)
        self.tar.add(self.files.dumpimages.path, self.files.dumpimages.name)
        dumpsize = get_size(self.files.dumpimages.path)
        logging.debug("%s: dumpimages: %dBtyes" % (self.lmtask_id, dumpsize))
        if not self.dump_volumes():
            return False

        if self.migrate_opts['dockerfileModes'] == 'layer':
            # 只迁移可读写的容器层
            if not self.dump_diff():
                return False

        elif self.migrate_opts['dockerfileModes'] == 'images':
            # 打包容器的所有镜像，并发送
            if not self.dump_dockerimage():
                return False

        self.tar.close()
        settime(self.lmtask_id)
        tarsize = os.path.getsize(self.files.tarpath)
        logging.debug("%s: tar close size: %d" % (self.lmtask_id, tarsize))
        setstate_ofcontainer(self.container_name, TARED)

        # ----------------------------------SEND------------
        if self.migrate_opts['migratemode'] == 'pipeline':
            self.check_beforesend(restoreorder)
        settime(self.lmtask_id)
        if not self.send_file(self.files.tarpath, 'files'):
            return False
        settime(self.lmtask_id)
        setstate_ofcontainer(self.container_name, SENDED)

        # ----------------------------------RESTORE------------
        self.check_beforerestore(restoreorder)
        settime(self.lmtask_id)
        if self.migrate_opts['restore']:
            if not self.restore(self.container_name, restore_arguments_str, self.new_imagename, restore_cmd_str):
                return False
        if not self.close():
            return False
        settime(self.lmtask_id)
        setstate_ofcontainer(self.container_name, RESTORED)

        logging.info("task id: %s finish" % self.lmtask_id)
        return True
