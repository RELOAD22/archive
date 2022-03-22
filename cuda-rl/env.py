import numpy as np
import time
import sys
import subprocess
from RL_agent import QLearningTable
import threading


# 环境包括：1.当前设备可利用资源、2.请求流中当前的请求
class CudaEnv(object):
    def __init__(self, request_stream, core_num):
        super(CudaEnv, self).__init__()
        self.jobs = request_stream
        self.core_num = core_num
        self.cur_state = 0
        self.acc_reward = 0
        self.nn_types = {}
        self.state_space = []
        self.action_space = []
        self.job_queue = []
        self.action_group_size = self.core_num + 1
        self.state_group_size = self.core_num + 1
        self._build()

    def _build(self):
        for item in self.jobs:
            self.nn_types[item[0]] = True
        # 一个job对应的状态点,每个作业对应0~7个当前可用资源

        # state 0 : 第一个作业（alexnet），n个剩余资源
        # state 1 : 第一个作业（alexnet），n-1个剩余资源
        # ...
        # state state_group_size : 第2个作业（googlenet），n个剩余资源
        # ...
        self.state_space = [i for i in range(
            0, self.state_group_size * len(self.jobs))]
        # action 0: alexnet, 分配资源0
        # action 1： alexnet，分配资源1
        # action n： alexnet, 分配资源n
        # ...
        # action action_group_size, google, 分配资源1
        # ...
        self.action_space = [i for i in range(
            0, self.action_group_size * len(self.nn_types))]

        # TODO 获取真正执行时间
        self.alex_512_runtime_table = {
            1: 0.307,
            2: 0.168,
            3: 0.122,
            4: 0.090,
            5: 0.062,
            6: 0.052,
            7: 0.047,
        }

        self.alex_128_runtime_table = {
            1: 0.076,
            2: 0.042,
            3: 0.030,
            4: 0.022,
            5: 0.015,
            6: 0.013,
            7: 0.012,
        }

        self.google_256_runtime_table = {
            1: 0.326,
            2: 0.171,
            3: 0.120,
            4: 0.085,
            5: 0.053,
            6: 0.043,
            7: 0.023,
        }

    # 根据state id获取对应空余资源
    def _get_free_core_by_sid(self, state_id):
        free_core = self.core_num - int(state_id % self.state_group_size)
        return free_core

    # 根据state id获取当前处理作业的网络类型、空余资源、batch size
    def _get_states_info(self, state_id):
        job_id = int(state_id / self.state_group_size)
        nn = self.jobs[job_id][0]
        batch_size = self.jobs[job_id][1]
        # 从小到大
        free_core = self._get_free_core_by_sid(state_id)
        return (nn, free_core, batch_size)

    def _action2core(self, action):
        core = action % self.action_group_size
        return core

    def _run_job(self, job_queue, action, RL, wait_times, observation, observation_):
        #child = subprocess.Popen('ls', shell=True)
        core = self._action2core(action)
        if core == 0:
            return 0
        info = self._get_states_info(self.cur_state)
        nn = info[0]
        bs = info[2]

        print("[RUN] new thread created.")
        start_time = time.time()
        print("[JOB] append job_queue:", 'RUNNING', core, start_time)
        self.job_queue.append(('RUNNING', core, start_time))

        child = subprocess.Popen(
            ['./runjob.bash', str(core), str(nn), str(bs)])
        child.wait()

        end_time = time.time()
        spend = end_time - start_time

        self.job_queue.remove(('RUNNING', core, start_time))
        self.job_queue.append(('WAITING', core, start_time))

        reward = 1/(core+1) + 1/(spend+1) + 1/(wait_times+1)
        RL.learn(observation, action, reward, observation_)
        self.acc_reward += reward

    def _fake_run_job(self, action):
        core = self._action2core(action)
        if core == 0:
            return 0
        info = self._get_states_info(self.cur_state)
        nn = info[0]
        bs = info[2]
        if nn == "AlexNet":
            if bs == 128:
                return self.alex_128_runtime_table[core]
            else:
                return self.alex_512_runtime_table[core]
        else:
            return self.google_256_runtime_table[core]
    '''
    def _run_job(self, job_id):
        pass'''

    def _real_wait_free_core(self):
        pass

    # 记录历史执行作业(作业执行时间,作业占用资源)
    def _wait_free_core(self):
        print(self.job_queue[0])
        core = self.job_queue[0][1]
        state = self.job_queue[0][0]  # 'RUNNING'/'WAITING
        start_time = self.job_queue[0][2]

        temp_queue = self.job_queue
        begin_wait_time = time.time()
        while True:
            for job in temp_queue:
                cur_time = time.time()
                core = job[1]
                state = job[0]
                start_time = job[2]
                if state == 'WAITING':
                    print("[JOB]: free job info: cores:%d, state:%s, cur_time-start_time:%f" %
                          (core, state, cur_time-start_time))
                    self.job_queue.remove(job)
                    print("current jobs:", len(self.job_queue))
                    return (core, cur_time - begin_wait_time)
                time.sleep(0.01)
        '''        
        else:
            print("_wait_free_core: no free job ")
            return (0,1)'''

    def reset(self):
        self.cur_state = 0
        self.acc_reward = 0
        self.state_trans = []
        self.action_trans = []
        self.job_queue = []

    def render(self):
        self.reset()
        return self.cur_state

    def step(self, action, RL):
        a2c = self._action2core(action)
        wait_times = 0
        free_core = self._get_states_info(self.cur_state)[1]
        if a2c == 0 or a2c % self.action_group_size == 0:
            free_info = self._wait_free_core()
            free_core += free_info[0]
            wait_times += free_info[1]
            state_ = int(int(self.cur_state / self.state_group_size)
                         * self.state_group_size + self.core_num - free_core)
        else:
            free_core = self._get_states_info(self.cur_state)[1]
            state_ = int(int(self.cur_state / self.state_group_size + 1)
                         * self.state_group_size + self.core_num - (free_core - a2c))
            print("[STATE] state change: cur_state:%d -> new_state:%d   free_core(old):%d occupy_cores:%d " %
                  (self.cur_state, state_, free_core, a2c))

        print("[RUN] ", self._get_states_info(self.cur_state))
        #spend = self._fake_run_job(action)
        #self._run_job(action, RL, wait_times, self.cur_state, state_)
        if a2c > 0:
            thread_job = threading.Thread(target=self._run_job, args=(
                self.job_queue, action, RL, wait_times, self.cur_state, state_))
            thread_job.start()
        else:
            thread_job = 0

        self.cur_state = state_

        #reward = 1/(a2c+1) + 1/(spend+1) + 1/(wait_times+1)
        done = state_ >= len(self.jobs) * self.state_group_size

        print("[JOB] job_queue:(latency, cores, start_time)", self.job_queue)
        return state_, thread_job, done
