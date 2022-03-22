from env import CudaEnv
from RL_agent import QLearningTable
import time

ITERATION = 200


def update():
    for episode in range(ITERATION):
        # initial observation
        observation = env.render()

        env.reset()
        print("======================== episode:%d =======================" % (episode))
        threads = []
        while True:
            # fresh env
            # env.reset()

            # RL choose action based on observation
            action = RL.choose_action(observation)
            print("[STATE] current state:%d    choose action_id:%d" %
                  (observation, action))

            # RL take action and get next observation and reward
            observation_, thread_job, done = env.step(action, RL)

            print("[STATE] new state:%d " % (observation_))

            if thread_job:
                threads.append(thread_job)
                print("[CONTROL] job thread(%d) added" % (len(threads)))

            # RL learn from this transition
            #RL.learn(observation, action, reward, observation_)

            # swap observation
            observation = observation_

            print("")
            # time.sleep(0.1)
            # break while loop when end of this episode
            if done:
                break

        for thread_job in threads:
            thread_job.join()
            print("[CONTROL] thread join")

        print("[!!!!!!!!!!] acc_reward:%f" % (env.acc_reward))

        time.sleep(1)

        if(episode % 20 == 0):
            RL.save_to_csv(episode)

    # end of game
    print('game over')
    # env.destroy()


if __name__ == "__main__":
    nn_types = ["AlexNet", "GoogleNet"]

    # Request Stream
    jobs = [
        ("AlexNet", 256),
        ("GoogleNet", 256),
        ("AlexNet", 128),
        ("GoogleNet", 256),
    ]
    core_num = 10
    env = CudaEnv(jobs, core_num)
    RL = QLearningTable(jobs, env.action_space, len(
        env.action_space), len(env.state_space), core_num)

    update()
    RL.save_to_csv(ITERATION)
