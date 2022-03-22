import docker
import logging


class DockerContainer:
    def __init__(self):
        self.cli = docker.APIClient()

    def get_container_info(self, container_name):
        # return list
        container_info = self.cli.containers(
            all=True, filters={"name": container_name})
        if not container_info:
            logging.error('No container: %s. Please check.' % container_name)
            return False

        container_info = container_info[0]
        if container_info['State'] != 'running':
            logging.info('container %s\'s status is %s(not running)' %
                         (container_name, container_info['State']))

        return container_info

    def get_container_id(self, container_name):
        # return list
        container_info = self.cli.containers(
            all=True, filters={"name": container_name})
        if not container_info:
            logging.error('No container: %s. Please check.' % container_name)
            return False

        container_info = container_info[0]

        return container_info['Id']

    def get_container_memsize(self, container_name):
        '''get memory size of container.
            return bytes    
        '''
        container_stats = self.cli.stats(container_name, stream=False)
        if not container_stats:
            logging.error('No container stats: %s. Please check.' %
                          container_name)
            return False

        mem_size = container_stats['memory_stats']['stats']['active_anon']

        return mem_size

    def get_container_image(self, container_name):
        # return list
        '''
        container_info = self.cli.containers(all = True, filters = {"name" : container_name})
        if not container_info:
            logging.error('No container: %s. Please check.'%container_name)
            return False

        container_info = container_info[0] 

        return container_info['Image']'''
        # return dict
        inspect = self.cli.inspect_container(container_name)
        if not inspect:
            logging.error('No container: %s. Please check.' % container_name)
            return False
        return inspect['Config']['Image']

    def get_container_UpperDir(self, container_name):
        # return dict
        inspect = self.cli.inspect_container(container_name)
        if not inspect:
            logging.error('No container: %s. Please check.' % container_name)
            return False
        UpperDir = inspect["GraphDriver"]['Data']["UpperDir"]

        return UpperDir

    #----Check whether the container is running or not.----#
    def check_container_running(self, container_id):
        # return list
        container_info = self.cli.containers(
            all=True, filters={"id": container_id})
        if not container_info:
            logging.error('No container: %s. Please check.' % container_id)
            return False

        container_info = container_info[0]
        if container_info['State'] != 'running':
            return False
        else:
            return True

    def set_arguments_fromconfig(self, migrate_opts, taskid):
        # return dict
        inspect = self.cli.inspect_container(migrate_opts['name'])
        if not inspect:
            logging.error('No container: %s. Please check.' %
                          migrate_opts['name'])
            return False
        # -----获取docker一般创建参数---
        arguments = []

        if inspect['Config']['AttachStdin'] or \
                inspect['Config']['OpenStdin'] or \
                inspect['Config']['StdinOnce']:
            arguments.append('-i')

        if inspect['Config']['Tty']:
            arguments.append('-t')

        ports = inspect['NetworkSettings']['Ports']
        if ports:
            for k, v in ports.items():
                if v:
                    hostport = v[0]['HostPort']
                    portarg = "-p " + hostport + ":" + k
                    arguments.append(portarg)

        # -----获取容器初始运行命令----
        Cmd = inspect['Config']['Cmd']
        for number in range(len(Cmd)):
            if Cmd[number].find(' ') != -1:
                # 如果有空格，则需要在首位加上'
                Cmd[number] = '\'' + Cmd[number] + '\''

        # ------镜像名（与创建时一致）---
        Image = inspect['Config']['Image']

        # ------处理容器卷挂载------
        mountinfo = inspect['Mounts']
        if migrate_opts['volumes'] == 'All':
            for count in range(len(mountinfo)):
                tarvol_name = 'volume-' + str(count)
                Source = mountinfo[count]['Source']
                Destination = mountinfo[count]['Destination']
                mountinfo[count]['Newname'] = tarvol_name
                mountinfo[count]['Migrate'] = True
                arguments.append('-v ' + '$PWD/' +
                                 tarvol_name + ':' + Destination)
        elif migrate_opts['volumes'] == 'Part':
            for count in range(len(mountinfo)):
                Source = mountinfo[count]['Source']
                Destination = mountinfo[count]['Destination']
                if Destination not in migrate_opts['migrateVols']:
                    # 不是我们要迁移的卷，跳过
                    arguments.append('-v ' + Source + ':' + Destination)
                    continue
                tarvol_name = 'volume-' + str(count)
                mountinfo[count]['Newname'] = tarvol_name
                mountinfo[count]['Migrate'] = True
                arguments.append('-v ' + '$PWD/' + taskid +
                                 '/' + tarvol_name + ':' + Destination)
        else:
            for count in range(len(mountinfo)):
                Source = mountinfo[count]['Source']
                Destination = mountinfo[count]['Destination']
                mountinfo[count]['Migrate'] = False
                arguments.append('-v ' + Source + ':' + Destination)
        arguments.append('--security-opt seccomp:unconfined')

        restore_arguments_str = ' '
        for i in arguments:
            restore_arguments_str += ' ' + i

        restore_cmd_str = ' '
        for i in Cmd:
            restore_cmd_str += ' ' + i

        logging.debug("parse arguments from container config:%s %s %s"
                      % (restore_arguments_str, Image, restore_cmd_str))

        return restore_arguments_str, restore_cmd_str, mountinfo


dockerclient = DockerContainer()
