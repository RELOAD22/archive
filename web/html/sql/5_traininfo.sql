/*
  tr   | sn1  | stn1 | sn2  | stn2 |     dtime      |     atime      |  hp   | 硬座 | sp | 软座 |  hst  | 硬卧上 |  hsm   | 硬卧中 |  hsb   | 硬卧下 |  sst   | 软卧上 |  ssb   | 软卧下 
-------+------+------+------+------+----------------+----------------+-------+------+----+------+-------+--------+--------+--------+--------+--------+--------+--------+--------+--------
 K1512 | 南京 |   12 | 徐州 |   13 | 20:38:00       | 1 day 02:46:00 | 26.00 |    5 |    |    0 | 44.00 |      5 |  47.00 |      5 |  47.00 |      5 |  73.00 |      5 |  74.00 |      5
 K162  | 南京 |   21 | 徐州 |   25 | 1 day 14:56:00 | 1 day 19:59:00 | 30.00 |    5 |    |    0 | 51.00 |      5 |  53.00 |      5 |  58.00 |      5 |  82.00 |      5 |  92.00 |      5
 Z176  | 南京 |    8 | 徐州 |   11 | 18:48:00       | 21:49:00       | 36.00 |    5 |    |    0 | 61.00 |      5 |  63.00 |      5 |  69.00 |      5 |  99.00 |      5 | 111.00 |      5
*/
SELECT * 
FROM(
SELECT TRAINT.TR, TRAINT.SN1, STN1, TRAINT.SN2, STN2,
    concat(DATE_PART('hour',DTIME),':', DATE_PART('minute',DTIME))::time AS DTIMET,
    concat(DATE_PART('hour',ATIME),':', DATE_PART('minute',ATIME))::time AS ATIMET,
    HP ,
    SP ,
    HST,
    HSM,
    HSB,
    SST,
    SSB,
    (CASE WHEN HP  IS NULL THEN 0 ELSE 硬座   END) AS 硬座  ,
    (CASE WHEN SP  IS NULL THEN 0 ELSE 软座   END) AS 软座  ,
    (CASE WHEN HST IS NULL THEN 0 ELSE 硬卧上 END) AS 硬卧上,
    (CASE WHEN HSM IS NULL THEN 0 ELSE 硬卧中 END) AS 硬卧中,
    (CASE WHEN HSB IS NULL THEN 0 ELSE 硬卧下 END) AS 硬卧下,
    (CASE WHEN SST IS NULL THEN 0 ELSE 软卧上 END) AS 软卧上,
    (CASE WHEN SSB IS NULL THEN 0 ELSE 软卧下 END) AS 软卧下,
    LEAST(HP ,SP ,HST,HSM,HSB,SST,SSB) AS LEASTP
FROM 
(
    SELECT T1.TRAIN AS TR, T1.SNAME AS SN1, T2.SNAME AS SN2, T1.DTIME AS DTIME, T2.ATIME AS ATIME,
        (T2.HP  -T1.HP  ) AS HP ,
        (T2.SP  -T1.SP  ) AS SP ,
        (T2.HST -T1.HST ) AS HST,
        (T2.HSM -T1.HSM ) AS HSM,
        (T2.HSB -T1.HSB ) AS HSB,
        (T2.SST -T1.SST ) AS SST,
        (T2.SSB -T1.SSB ) AS SSB,
        JUSTIFY_INTERVAL(T2.ATIME-T1.DTIME) AS INTERVAL2
    FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2, STATION AS S1, STATION AS S2
    WHERE /*T1和T2是所有南京到徐州直达列车车站的组合*/
        T1.TRAIN = T2.TRAIN 
        AND T1.SNAME = S1.SNAME
        AND T2.SNAME = S2.SNAME
        AND S1.CNAME = $1 AND S2.CNAME = $2
        AND T1.STOPNUM < T2.STOPNUM 
        AND concat(DATE_PART('hour',T1.DTIME),':', DATE_PART('minute',T1.DTIME))::time > $4
    ORDER BY HP 
) AS TRAINI,
(
    SELECT TR,STN1,STN2,SN1,SN2,
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
        T1.SNAME AS SN1, T2.SNAME AS SN2,
        TICKETTYPE, COUNT(*) AS TCOUNT
    FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2,  STATION AS S1, STATION AS S2, SEAT AS SE
    WHERE 
        	T1.SNAME = S1.SNAME AND T2.SNAME = S2.SNAME /*T1出发 T2到达*/
        AND T1.TRAIN = T2.TRAIN /*T1T2是一列车不同站点*/
        AND S1.CNAME = $1 AND S2.CNAME = $2 /**/
        AND T1.STOPNUM < T2.STOPNUM
    	AND SEATID NOT IN (
            SELECT SEATID
            FROM TCK_STOPNUM AS TCK
            WHERE 
                    TICKET_DATE = $3
                AND TCK.TRAIN = T1.TRAIN
                AND (   /*T1为出发站点 T2为到达站点*/
                        (STOPNUM1 <= T1.STOPNUM AND T1.STOPNUM < STOPNUM1)
                        OR
                        (STOPNUM1 < T2.STOPNUM AND T2.STOPNUM <= STOPNUM2))
    	)   
    GROUP BY TR,STN1,STN2,SN1,SN2,TICKETTYPE
    ) AS RESULT
    GROUP BY TR,STN1,STN2,SN1,SN2
    ORDER BY TR
) AS TRAINT
WHERE TRAINI.TR = TRAINT.TR
    AND TRAINI.SN1 = TRAINT.SN1
    AND TRAINI.SN2 = TRAINT.SN2
ORDER BY LEASTP
) AS RESULT
WHERE 
       硬座   > 0
    OR 软座   > 0
    OR 硬卧上 > 0
    OR 硬卧中 > 0
    OR 硬卧下 > 0
    OR 软卧上 > 0
    OR 软卧下 > 0
ORDER BY LEASTP  LIMIT 10
;




