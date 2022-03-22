/*
  tr   | sn1  | sn2  | t1硬座 | t1软座 | t1硬卧上 | t1硬卧中 | t1硬卧下 | t1软卧上 | t1软卧下 |  hp1  | sp1 | hst1  | hsm1  | hsb1  | sst1  | ssb1  |  tr   |   sn3    |  sn4   | t2硬座 | t2软座 | t2硬卧上 | t2硬卧中 | t2硬卧下 | t2软卧上 | t2软卧下 |  hp2   |  sp2   | hst2 | hsm2 | hsb2 | sst2 | ssb2 | leastp 
-------+------+------+--------+--------+----------+----------+----------+----------+----------+-------+-----+-------+-------+-------+-------+-------+-------+----------+--------+--------+--------+----------+----------+----------+----------+----------+--------+--------+------+------+------+------+------+--------
 K284  | 南京 | 常州 |      5 |      0 |        5 |        5 |        5 |        5 |        5 |  1.00 |     |  1.00 |  1.00 |  1.00 |  1.00 |  1.00 | G456  | 常州北   | 徐州东 |      5 |      0 |        0 |        0 |        0 |        0 |        0 | 204.50 |        |      |      |      |      |      | 205.50
 K284  | 南京 | 常州 |      5 |      0 |        5 |        5 |        5 |        5 |        5 |  1.00 |     |  1.00 |  1.00 |  1.00 |  1.00 |  1.00 | G12   | 常州北   | 徐州东 |      5 |      0 |        0 |        0 |        0 |        0 |        0 | 204.50 |        |      |      |      |      |      | 205.50
 K284  | 南京 | 常州 |      5 |      0 |        5 |        5 |        5 |        5 |        5 |  1.00 |     |  1.00 |  1.00 |  1.00 |  1.00 |  1.00 | G110  | 常州北   | 徐州东 |      5 |      0 |        0 |        0 |        0 |        0 |        0 | 204.50 |        |      |      |      |      |      | 205.50
*/

DROP VIEW TRAIN2I1;
DROP VIEW TRAIN2I2;
DROP VIEW TRAIN2I;

CREATE VIEW TRAIN2I AS 
SELECT 
    *, INTERVAL + DTIME3 - ATIME2 AS INTERVALALL
FROM(
SELECT 
    T1.TRAIN AS TR1, T1.SNAME AS SN1, T2.SNAME AS SN2, T1.STOPNUM AS STN1, T2.STOPNUM AS STN2, 
    T3.TRAIN AS TR2, T3.SNAME AS SN3, T4.SNAME AS SN4, T3.STOPNUM AS STN3, T4.STOPNUM AS STN4,

    (T2.HP  -T1.HP  ) AS HP1 ,
    (T2.SP  -T1.SP  ) AS SP1 ,
    (T2.HST -T1.HST ) AS HST1,
    (T2.HSM -T1.HSM ) AS HSM1,
    (T2.HSB -T1.HSB ) AS HSB1,
    (T2.SST -T1.SST ) AS SST1,
    (T2.SSB -T1.SSB ) AS SSB1,
    (T4.HP  -T3.HP  ) AS HP2 ,
    (T4.SP  -T3.SP  ) AS SP2 ,
    (T4.HST -T3.HST ) AS HST2,
    (T4.HSM -T3.HSM ) AS HSM2,
    (T4.HSB -T3.HSB ) AS HSB2,
    (T4.SST -T3.SST ) AS SST2,
    (T4.SSB -T3.SSB ) AS SSB2,
    LEAST(T2.HP  -T1.HP 
        ,T2.SP  -T1.SP 
        ,T2.HST -T1.HST
        ,T2.HSM -T1.HSM
        ,T2.HSB -T1.HSB
        ,T2.SST -T1.SST
        ,T2.SSB -T1.SSB)+
    LEAST(T4.HP  -T3.HP 
        ,T4.SP  -T3.SP 
        ,T4.HST -T3.HST
        ,T4.HSM -T3.HSM
        ,T4.HSB -T3.HSB
        ,T4.SST -T3.SST
        ,T4.SSB -T3.SSB) AS LEASTP,

    JUSTIFY_INTERVAL((T2.ATIME-T1.DTIME)+(T4.ATIME-T3.DTIME)) AS INTERVAL,
    CONCAT(DATE_PART('hour',T1.DTIME),':', DATE_PART('minute',T1.DTIME))::time AS DTIME1,
    CONCAT(DATE_PART('hour',T2.ATIME),':', DATE_PART('minute',T2.ATIME))::time AS ATIME2,
    CONCAT(DATE_PART('hour',T3.DTIME),':', DATE_PART('minute',T3.DTIME))::time AS DTIME3,
    CONCAT(DATE_PART('hour',T4.ATIME),':', DATE_PART('minute',T4.ATIME))::time AS ATIME4

FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2, STATION AS S1, STATION AS S2
    ,TRAIN_STATION AS T3, TRAIN_STATION AS T4, STATION AS S3, STATION AS S4
WHERE /*T1T2 T3T4是所有南京到徐州换乘一次列车的组合*/
        T1.TRAIN = T2.TRAIN
    AND T3.TRAIN = T4.TRAIN
    AND S1.SNAME = T1.SNAME
    AND S2.SNAME = T2.SNAME
    AND S3.SNAME = T3.SNAME
    AND S4.SNAME = T4.SNAME
    AND S2.CNAME = S3.CNAME 
    AND S1.CNAME = $1 AND S4.CNAME = $2
    AND T1.STOPNUM < T2.STOPNUM
    AND T3.STOPNUM < T4.STOPNUM
    AND (   
            (   T2.SNAME=T3.SNAME 
                AND DATE_PART('hour',T3.DTIME)*60 BETWEEN DATE_PART('hour',T2.ATIME)*60+60 AND DATE_PART('hour',T2.ATIME)*60+240
            )
            OR DATE_PART('hour',T3.DTIME)*60 BETWEEN DATE_PART('hour',T2.ATIME)*60+120 AND DATE_PART('hour',T2.ATIME)*60+240 
        )
    AND DATE_PART('hour',T1.DTIME) >= $5
ORDER BY LEASTP, INTERVAL, T1.DTIME
) AS RESULT
ORDER BY LEASTP, INTERVALALL, DTIME1 LIMIT 20
;


CREATE VIEW TRAIN2I1 AS 
SELECT DISTINCT
    TR1, SN1, SN2, STN1, STN2 
FROM TRAIN2I
;

CREATE VIEW TRAIN2I2 AS 
SELECT DISTINCT
    TR2, SN3, SN4, STN3, STN4
FROM TRAIN2I
;

SELECT *
FROM
(
SELECT 
    TRAIN2I.TR1, TRAIN2I.SN1, TRAIN2I.SN2, TRAIN2I.DTIME1, TRAIN2I.ATIME2,
    (CASE WHEN HP1  IS NULL THEN 0 ELSE TRAIN1.硬座   END) AS T1硬座  ,
    (CASE WHEN SP1  IS NULL THEN 0 ELSE TRAIN1.软座   END) AS T1软座  ,
    (CASE WHEN HST1 IS NULL THEN 0 ELSE TRAIN1.硬卧上 END) AS T1硬卧上,
    (CASE WHEN HSM1 IS NULL THEN 0 ELSE TRAIN1.硬卧中 END) AS T1硬卧中,
    (CASE WHEN HSB1 IS NULL THEN 0 ELSE TRAIN1.硬卧下 END) AS T1硬卧下,
    (CASE WHEN SST1 IS NULL THEN 0 ELSE TRAIN1.软卧上 END) AS T1软卧上,
    (CASE WHEN SSB1 IS NULL THEN 0 ELSE TRAIN1.软卧下 END) AS T1软卧下,
    HP1 ,
    SP1 ,
    HST1,
    HSM1,
    HSB1,
    SST1,
    SSB1,
    TRAIN2I.TR2, TRAIN2I.SN3, TRAIN2I.SN4, TRAIN2I.DTIME3, TRAIN2I.ATIME4,
    (CASE WHEN HP2  IS NULL THEN 0 ELSE TRAIN2.硬座   END) AS T2硬座  ,
    (CASE WHEN SP2  IS NULL THEN 0 ELSE TRAIN2.软座   END) AS T2软座  ,
    (CASE WHEN HST2 IS NULL THEN 0 ELSE TRAIN2.硬卧上 END) AS T2硬卧上,
    (CASE WHEN HSM2 IS NULL THEN 0 ELSE TRAIN2.硬卧中 END) AS T2硬卧中,
    (CASE WHEN HSB2 IS NULL THEN 0 ELSE TRAIN2.硬卧下 END) AS T2硬卧下,
    (CASE WHEN SST2 IS NULL THEN 0 ELSE TRAIN2.软卧上 END) AS T2软卧上,
    (CASE WHEN SSB2 IS NULL THEN 0 ELSE TRAIN2.软卧下 END) AS T2软卧下,
    HP2 ,
    SP2 ,
    HST2,
    HSM2,
    HSB2,
    SST2,
    SSB2,
    LEASTP
FROM
TRAIN2I,
( 
    SELECT TR,STN1,STN2,
          MAX(CASE TICKETTYPE WHEN '硬座'   THEN TCOUNT ELSE 0 END) AS 硬座  ,
          MAX(CASE TICKETTYPE WHEN '软座'   THEN TCOUNT ELSE 0 END) AS 软座  ,
          MAX(CASE TICKETTYPE WHEN '硬卧上' THEN TCOUNT ELSE 0 END) AS 硬卧上,
          MAX(CASE TICKETTYPE WHEN '硬卧中' THEN TCOUNT ELSE 0 END) AS 硬卧中,
          MAX(CASE TICKETTYPE WHEN '硬卧下' THEN TCOUNT ELSE 0 END) AS 硬卧下,
          MAX(CASE TICKETTYPE WHEN '软卧上' THEN TCOUNT ELSE 0 END) AS 软卧上,
          MAX(CASE TICKETTYPE WHEN '软卧下' THEN TCOUNT ELSE 0 END) AS 软卧下
    FROM(
        SELECT 
            T1.TRAIN AS TR, 
            T1.STOPNUM AS STN1, T2.STOPNUM AS STN2, 
            TICKETTYPE, COUNT(*) AS TCOUNT
        FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2, SEAT AS SE,
            TRAIN2I1
        WHERE 
                T1.TRAIN = T2.TRAIN /*T1T2是一列车不同站点*/
            AND T1.TRAIN = TRAIN2I1.TR1
            AND T1.STOPNUM = TRAIN2I1.STN1 AND T2.STOPNUM = TRAIN2I1.STN2
            AND T1.STOPNUM < T2.STOPNUM
        	AND SEATID NOT IN (
                SELECT SEATID
                FROM TCK_STOPNUM AS TCK
                WHERE 
                        TICKET_DATE = DATE $4
                    AND TCK.TRAIN = T1.TRAIN
                    AND (   /*T1为出发站点 T2为到达站点*/
                            (STOPNUM1 <= T1.STOPNUM AND T1.STOPNUM < STOPNUM1)
                            OR
                            (STOPNUM1 < T2.STOPNUM AND T2.STOPNUM <= STOPNUM2))
        	)
        GROUP BY TR,T1.STOPNUM,T2.STOPNUM,TICKETTYPE
    ) AS RESULT
    GROUP BY TR,STN1,STN2
    ORDER BY TR
) AS TRAIN1,
(
    SELECT TR,STN1,STN2,
          MAX(CASE TICKETTYPE WHEN '硬座'   THEN TCOUNT ELSE 0 END) AS 硬座  ,
          MAX(CASE TICKETTYPE WHEN '软座'   THEN TCOUNT ELSE 0 END) AS 软座  ,
          MAX(CASE TICKETTYPE WHEN '硬卧上' THEN TCOUNT ELSE 0 END) AS 硬卧上,
          MAX(CASE TICKETTYPE WHEN '硬卧中' THEN TCOUNT ELSE 0 END) AS 硬卧中,
          MAX(CASE TICKETTYPE WHEN '硬卧下' THEN TCOUNT ELSE 0 END) AS 硬卧下,
          MAX(CASE TICKETTYPE WHEN '软卧上' THEN TCOUNT ELSE 0 END) AS 软卧上,
          MAX(CASE TICKETTYPE WHEN '软卧下' THEN TCOUNT ELSE 0 END) AS 软卧下
    FROM(
        SELECT 
            T1.TRAIN AS TR, 
            T1.STOPNUM AS STN1, T2.STOPNUM AS STN2, 
            TICKETTYPE, COUNT(*) AS TCOUNT
        FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2, SEAT AS SE,
            TRAIN2I2
        WHERE 
                T1.TRAIN = T2.TRAIN /*T1T2是一列车不同站点*/
            AND T1.TRAIN = TRAIN2I2.TR2
            AND T1.STOPNUM = TRAIN2I2.STN3 AND T2.STOPNUM = TRAIN2I2.STN4
            AND T1.STOPNUM < T2.STOPNUM
        	AND SEATID NOT IN (
                SELECT SEATID
                FROM TCK_STOPNUM AS TCK
                WHERE 
                        TICKET_DATE = DATE $4
                    AND TCK.TRAIN = T1.TRAIN
                    AND (   /*T1为出发站点 T2为到达站点*/
                            (STOPNUM1 <= T1.STOPNUM AND T1.STOPNUM < STOPNUM1)
                            OR
                            (STOPNUM1 < T2.STOPNUM AND T2.STOPNUM <= STOPNUM2))
        	)
        GROUP BY TR,T1.STOPNUM,T2.STOPNUM,TICKETTYPE
    ) AS RESULT
    GROUP BY TR,STN1,STN2
    ORDER BY TR
) AS TRAIN2
WHERE 
        TRAIN2I.TR1 = TRAIN1.TR 
    AND TRAIN2I.TR2 = TRAIN2.TR
    AND TRAIN2I.STN1 = TRAIN1.STN1 AND TRAIN2I.STN2 = TRAIN1.STN2
    AND TRAIN2I.STN3 = TRAIN2.STN1 AND TRAIN2I.STN4 = TRAIN2.STN2
ORDER BY LEASTP
) AS RESULT
WHERE (
           T1硬座   > 0
        OR T1软座   > 0
        OR T1硬卧上 > 0
        OR T1硬卧中 > 0
        OR T1硬卧下 > 0
        OR T1软卧上 > 0
        OR T1软卧下 > 0
    )
    AND (
           T2硬座   > 0
        OR T2软座   > 0
        OR T2硬卧上 > 0
        OR T2硬卧中 > 0
        OR T2硬卧下 > 0
        OR T2软卧上 > 0
        OR T2软卧下 > 0
    )
ORDER BY LEASTP LIMIT 10
;



