from lm_container import dockerclient
from lm_util import *
import logging


class MigrateTools:
    def time_predict(migrate_opts):
        '''
        predict time of migrate process
        return s
        '''
        mem_size = dockerclient.get_container_memsize(migrate_opts['name'])
        send_rate = rate_translate(migrate_opts['sendRate'])
        if send_rate == False:
            logging.error(
                "prase send rate error. please check sendRate in config")
            return False
        send_time = (mem_size * 8) / send_rate
        logging.info("mem size:%d  send rate:%dbps send time:%ds" %
                     (mem_size, send_rate, send_time))

        total_time = send_time
        print('container:%s  estimated time:%ds' %
              (migrate_opts['name'], total_time))
        return total_time
