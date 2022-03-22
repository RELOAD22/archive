"""
This part of code is the Q learning brain, which is a brain of the agent.
All decisions are made in here.
"""

import numpy as np


class QLearningTable:
    def __init__(self, jobs, actions, action_size, state_size, core_num, learning_rate=0.1, reward_decay=0.8, e_greedy=0.8):
        self.lr = learning_rate
        self.gamma = reward_decay
        self.epsilon = e_greedy
        self.q_table = np.zeros([action_size, state_size])
        self.core_num = core_num
        self.action_group_size = self.core_num + 1
        self.state_group_size = self.core_num + 1
        self.actions = actions
        self.jobs = jobs
        self.state_size = state_size
        # TODO 可配置化， 用于快速查找某个模型的可使用动作空间
        self.action_range = {
            "AlexNet": (0, self.action_group_size),
            "GoogleNet": (self.action_group_size, 2 * self.action_group_size)
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

    def _get_maxq_choice(self, filter_actions, observation):
        max_q = -1
        ret_actions = []
        for pa in filter_actions:
            if max_q <= self.q_table[pa, observation]:
                ret_actions.append(pa)
        return ret_actions

    def choose_action(self, observation):
        # self.check_state_exist(observation)
        state_info = self._get_states_info(observation)
        nn = state_info[0]
        free_core = state_info[1]
        possible_actions = self.actions[self.action_range[nn]
                                        [0]: self.action_range[nn][1]]
        filter_actions = []
        if free_core == self.core_num:
            filter_actions = possible_actions[1:]
        else:
            for pa in possible_actions:
                ac = self._action2core(pa)
                if free_core >= ac:
                    filter_actions.append(pa)

        # action selection
        if np.random.uniform() < self.epsilon:
            # choose best action
            filter_actions = self._get_maxq_choice(filter_actions, observation)
            # some actions may have the same value, randomly choose on in these actions
            action = filter_actions[np.random.randint(0, len(filter_actions))]
        else:
            # choose random action
            action = filter_actions[np.random.randint(0, len(filter_actions))]
        return action

    def learn(self, s, a, r, s_):
        # self.check_state_exist(s_)
        #print("a:%d,s:%d"%(a, s))
        q_predict = self.q_table[a, s]
        if s_ < self.state_size:
            # next state is not terminal
            q_target = r + self.gamma * self.q_table[:, s_].max()
        else:
            q_target = r  # next state is terminal
        old_value = self.q_table[a, s]
        self.q_table[a, s] += self.lr * (q_target - q_predict)  # update
        print("[QTABLE] update q_table: loc[%d,%d] old_value:%f new_value:%f" % (
            a, s, old_value, self.q_table[a, s]))
    # def check_state_exist(self, state):
    #    if state:
    #    	return int(state)<32
    #    else:
    #        return false

    def save_to_csv(self, episode):
        Q = self.q_table
        q_table_name = "result/Q_table_" + str(episode) + ".csv"
        q_file = open(q_table_name, "w+")
        for i in range(0, len(Q[0])):
            q_file.write(str(self._get_states_info(i))+"\t")
        q_file.write("\n")
        for i in range(0, len(Q)):
            for j in range(0, len(Q[0])):
                q_file.write(str(Q[i, j]) + "\t")
            q_file.write("\n")
