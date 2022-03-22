
SELECT 
    DISTINCT TICKETTYPE
FROM TRAIN_STATION AS T1, TRAIN_STATION AS T2, SEAT AS SE
WHERE 
        T1.TRAIN = T2.TRAIN AND T1.TRAIN = $1/*T1T2是一列车不同站点*/
    AND T1.SNAME = $2 AND T2.SNAME = $3 /*T1出发 T2到达*/
    AND T1.STOPNUM < T2.STOPNUM
	AND SEATID NOT IN (
        SELECT SEATID
        FROM TCK_STOPNUM AS TCK
        WHERE 
                TICKET_DATE = $4
            AND TCK.TRAIN = T1.TRAIN
            AND (   /*T1为出发站点 T2为到达站点*/
                    (STOPNUM1 <= T1.STOPNUM AND T1.STOPNUM < STOPNUM1)
                    OR
                    (STOPNUM1 < T2.STOPNUM AND T2.STOPNUM <= STOPNUM2))
	)
;





