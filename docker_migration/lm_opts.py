#!/usr/bin
#encoding: utf-8
import os
import logging
migrate_opts = {
    "name": '',
    'destip': '',
    'port': 0,
    'dockerfileModes': 'layer',
    'volumes': False,
    'restore': False,
    'sendType': 'socket',
}


def opts_parser(args):
    # 容器名字
    migrate_opts["name"] = args.name

    migrate_opts['destip'] = args.destip
    migrate_opts['port'] = args.port

    migrate_opts['dockerfiles'] = args.dockerfiles
    migrate_opts['restore'] = args.restore
    migrate_opts['volumes'] = args.volumes

    return True
