#!/usr/bin/python
# -*- coding: utf-8 -*-
import pandas as pd
import csv
import os
csv_filepath = "../train-2016-10/0/1095.csv"
csv_savepath = "../train-changed/0/1095.csv"

for root, dirs, files in os.walk("/home/dbms/Lab2/train-2016-10"):
    for file in files:
        if (file.split('.')[-1] != 'csv'):
            continue
        filepath = os.path.join(root, file)
        # print(1)
        print(filepath)
        data = pd.read_csv(filepath, skipinitialspace=True, encoding='utf_8')
        # print(data.head(5))
        del data['停留（分）']
        del data['历时（分）']
        d_cols = ["stop_id", 'station_name', "atime", 'dtime', 'mileage',
                  "h/s", "h", 's']
        data.columns = d_cols
        data['train_id'] = filepath.split("/")[-1].split(".")[0]
        # print(data.head(5))

        # print(data["h/s"])
        names = data['h/s'].str.split('/', expand=True)
        names.columns = ['hp', 'sp']
        data = data.join(names)
        names = data['h'].str.split('/', expand=True)
        names.columns = ['hst', 'hsm', 'hsb']
        data = data.join(names)
        names = data['s'].str.split('/', expand=True)
        names.columns = ['sst', 'ssb']
        data = data.join(names)
        del data['h/s']
        del data['h']
        del data['s']
        order = ['train_id', 'station_name', 'stop_id', 'atime', 'dtime', 'mileage', 'hp',
                 'sp', 'hst', 'hsm', 'hsb', 'sst', 'ssb']
        data = data[order]
        data = data.replace("-", "NULL")
        data = data.fillna("NULL")
        for col in ['hp', 'sp', 'hst', 'hsm', 'hsb', 'sst', 'ssb']:
            data[col].replace("0", "NULL", inplace=True)
        # print(data.head(5))
        for col in ['hp', 'sp', 'hst', 'hsm', 'hsb', 'sst', 'ssb']:
            data.loc[0, col] = 0

        day_count = 0
        for row in data.index:
            if row != 0 and data.loc[row, 'atime'] != 'NULL' and data.loc[row-1, 'dtime'].split(' ')[-1] > data.loc[row, 'atime']:
                day_count += 1
            if data.loc[row, 'atime'] != 'NULL':
                data.loc[row, 'atime'] = str(
                    day_count) + ' D ' + data.loc[row, 'atime']
            if data.loc[row, 'atime'] != 'NULL' and data.loc[row, 'dtime'] != 'NULL' and data.loc[row, 'atime'].split(' ')[-1] > data.loc[row, 'dtime']:
                day_count += 1
            if data.loc[row, 'dtime'] != 'NULL':
                data.loc[row, 'dtime'] = str(
                    day_count) + ' D ' + data.loc[row, 'dtime']
        #dirs, filename = os.path.split(csv_savepath)
        path = "/home/dbms/train-changed/"
        if not os.path.exists(path + os.path.basename(root)):
            os.makedirs(path + os.path.basename(root))
        with open(path + os.path.basename(root) + '/' + file, 'w') as f:
            data.to_csv(f, encoding="utf-8", header=False, index=False)
        print('finish')
