import logging
import sys
import argparse
import time
from lmcheck import lm_check
import lm_migrate
from lm_migratetools import MigrateTools
from lm_opts import migrate_opts, opts_parser
from server import lm_service
from argparse import RawTextHelpFormatter
import yaml
import os
import threading
from lm_util import *

maxthreads = 2
dumporder = []
restoreorder = []
init_time = 0


def run(config):
    docker_migrate = lm_migrate.live_migrate(config)
    docker_migrate.run(dumporder, restoreorder)


def parase_args(args):
    if args.command == "check":
        lm_check()
    if args.command == "migrate":
        docker_migrate = lm_migrate.live_migrate(migrate_opts)
        docker_migrate.run()
    if args.command == "service":
        lm_service()
    if args.command == "restore":
        pass


def yaml_exec(args, migratemode, config_list):
    init_time = time.time()
    if args.command == 'migrate':
        '''
        for config in config_list:
                logging.debug("migrate_opt:%s"%(str(config)))
                docker_migrate = lm_migrate.live_migrate(config)
                docker_migrate.run()'''
        if migratemode == 'multithread' or migratemode == 'pipeline' or migratemode == 'default':
            '''
            thread_list = []
            for count in range(len(config_list)):
                    logging.debug("migrate_opt%d:%s"%(count, str(config_list[count])))
                    t = threading.Thread(target=run, args=(config_list[count],))
                    t.setDaemon(True)
                    thread_list.append(t)

            for t in thread_list:
                    t.start()
            for t in thread_list:
                    t.join()
            print("exec end")'''
            thread_list = []
            print(dumporder)
            for level in range(len(dumporder)):
                for container in dumporder[level]:
                    '''
                    if level > 0:
                            #循环直到上一等级所有容器进程被转储
                            flag = True
                            while flag:
                                    for c1 in dumporder[level - 1]:
                                            if getstate_ofcontainer(c1) < DUMPED:
                                                    flag = True
                                            else :
                                                    flag = False	'''
                    logging.debug("migrate_opt:%s" %
                                  (str(config_list[container])))
                    t = threading.Thread(
                        target=run, args=(config_list[container],))
                    t.setDaemon(True)
                    thread_list.append(t)
                    t.start()
            for t in thread_list:
                t.join()
            logging.debug("multithread exec finish")
        else:
            for config in config_list.values():
                logging.debug("migrate_opt:%s" % (str(config)))
                docker_migrate = lm_migrate.live_migrate(config)
                docker_migrate.run(dumporder, restoreorder)
        for k, v in gettime().items():
            print(k, end=' ')
            for x in [x-init_time for x in v]:
                print(x, end=' ')
            print('')

    if args.command == 'estimate':
        times = 0
        for config in config_list:
            logging.debug("migrate_opt:%s" % (str(config)))
            times += MigrateTools.time_predict(config)
        print('total estimated time: %s' % times)


'''
{'kind': 'share-config', 
'containers': ['name1', 'name2'], 
'config': 
	{'destip': '49.233.188.209', 
	'port': 10020, 
	'dockerfileModes': 'layer', 
	'volumes': False, 
	'restore': False}
}
    "name"  : '',
    'destip': '',
    'port'  : 0,
    'dockerfileModes' : 'layer',
    'volumes' : False,
    'restore' : False,
'''


def yaml_parser(configFile):
    f = open(configFile)
    # 设定工作目录，并切换至工作目录
    workpath = os.getcwd()+'/worktmp'
    logging.info('work path: %s' % workpath)
    isExists = os.path.exists(workpath)
    if not isExists:
        os.makedirs(workpath)
    os.chdir(workpath)
    config = yaml.load(f)
    config_list = {}
    for name in config['containers']:
        migrate_opts = {}
        migrate_opts['name'] = name
        migrate_opts['destip'] = config['config']['destip']
        migrate_opts['port'] = config['config']['port']
        migrate_opts['dockerfileModes'] = config['config']['dockerfileModes']
        migrate_opts['volumes'] = config['config']['volumes']
        migrate_opts['restore'] = config['config']['restore']
        migrate_opts['sendType'] = config['config']['sendType']
        migrate_opts['sendRate'] = config['config']['sendRate']
        migrate_opts['migrateVols'] = []
        #migrate_opts['debug'] = config['config']['debug']
        migrate_opts['migratemode'] = config['migratemode']
        if 'migrateVols' in config['config']:
            for config_vol in config['config']['migrateVols']:
                # looper1:/mnt/xfs/wgs-data/output
                config_vol_cn, config_vol_v = config_vol.split(':', 1)
                if name == config_vol_cn:
                    # 为该容器里的挂载卷
                    migrate_opts['migrateVols'].append(config_vol_v)

        config_list[name] = migrate_opts
        setstate_ofcontainer(name, INIT)
    global dumporder, restoreorder, maxthreads
    migratemode = config['migratemode']
    if migratemode == 'multithread' or migratemode == 'pipeline':
        dumporder = config['dumporder']
        restoreorder = config['restoreorder']
    return migratemode, config_list


if __name__ == '__main__':

    logging.basicConfig(
        level=logging.DEBUG, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    parser = argparse.ArgumentParser(
        description='docker live migration', formatter_class=RawTextHelpFormatter)

    parser.add_argument("command", type=str, choices=["migrate", "check", "service", "estimate"],
                        help='''Commands:
  migrate        checkpoint a container by name and restore on dst ip
  check          checks whether the kernel support is up-to-date
  service        launch service''',)
    parser.add_argument("-n", "--name", type=str,
                        help="docker container's name")

    parser.add_argument("-destip", type=str, help="dest ip")
    parser.add_argument("-port", type=int,
                        help="socket port. defualt 10020", default=10020)

    parser.add_argument(
        "-dockerfiles", choices=["layer", "images"], help="migrate container layer(rw) files(diff dir)")
    parser.add_argument("-volumes",  type=str,
                        choices=["None", "Part", "All"], help="migrate with volumes.")
    parser.add_argument("--restore", action='store_true',
                        help="migrate and restore.")

    parser.add_argument('--version', action='version',
                        version='docker lmt 0.1')

    parser.add_argument('-config', type=str, help="config file")

    args = parser.parse_args()
    if args.config:
        migratemode, config_list = yaml_parser(args.config)
        yaml_exec(args, migratemode, config_list)
    else:
        # 设定工作目录，并切换至工作目录
        workpath = os.getcwd()+'/worktmp'
        logging.info('work path: %s' % workpath)
        isExists = os.path.exists(workpath)
        if not isExists:
            os.makedirs(workpath)
        os.chdir(workpath)

        opts_parser(args)
        parase_args(args)
