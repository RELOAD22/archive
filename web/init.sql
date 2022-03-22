drop table STATION;

/*==============================================================*/
/* Table: STATION                                               */
/*==============================================================*/
create table STATION (
   SID                  INT2                 not null,
   SNAME                VARCHAR(20)          not null,
   CNAME                VARCHAR(20)          not null,
   constraint PK_STATION primary key (SNAME),
   constraint AK_KEY_2_STATION unique (SID)
);
drop table TRAIN_STATION;

/*==============================================================*/
/* Table: TRAIN_STATION                                         */
/*==============================================================*/
create table TRAIN_STATION (
   TRAIN                VARCHAR(20)          not null,
   SNAME                VARCHAR(20)          not null,
   STOPNUM              INT2                 not null,
   ATIME                INTERVAL             null,
   DTIME                INTERVAL             null,
   MILEAGE              INT2                 null,
   HP                   DECIMAL(10,2)        null,
   SP                   DECIMAL(10,2)        null,
   HST                  DECIMAL(10,2)        null,
   HSM                  DECIMAL(10,2)        null,
   HSB                  DECIMAL(10,2)        null,
   SST                  DECIMAL(10,2)        null,
   SSB                  DECIMAL(10,2)        null,
   constraint PK_TRAIN_STATION primary key (TRAIN, SNAME),
   constraint AK_KEY_2_TRAIN_ST unique (TRAIN, STOPNUM)
);

alter table TRAIN_STATION
   add constraint FK_TRAIN_ST_REFERENCE_STATION foreign key (SNAME)
      references STATION (SNAME)
      on delete restrict on update restrict;
drop table SEAT;
create type seattype as enum('硬座','软座','硬卧上','硬卧中','硬卧下','软卧上','软卧下');
/*==============================================================*/
/* Table: SEAT                                                  */
/*==============================================================*/
create table SEAT (
   SEATID               INT2                 not null,
   TICKETTYPE           seattype             not null,
   LOCATION             VARCHAR(10)          not null,
   constraint PK_SEAT primary key (SEATID),
   constraint AK_KEY_2_SEAT unique (TICKETTYPE, LOCATION)
);
drop table PASSENGER;

/*==============================================================*/
/* Table: PASSENGER                                             */
/*==============================================================*/
create table PASSENGER (
   USERNAME             VARCHAR(20)          not null,
   NAME                 VARCHAR(20)          null,
   IDNUM                VARCHAR(30)          null,
   PHONENUM             VARCHAR(20)          null,
   CARDNUM              VARCHAR(30)          null,
   PASSWORD             VARCHAR(20)          not null,
   constraint PK_PASSENGER primary key (USERNAME),
   constraint AK_KEY_2_PASSENGE unique (IDNUM),
   constraint AK_KEY_3_PASSENGE unique (PHONENUM)
);
drop table ORDERS;
create type orderstatus as enum('正常','已取消');
/*==============================================================*/
/* Table: ORDERS                                                */
/*==============================================================*/
create table ORDERS (
   ORDERID              VARCHAR(20)          not null,
   BOOK_DATE            TIMESTAMP            null,
   DSNAME               VARCHAR(20)          null,
   ASNAME               VARCHAR(20)          null,
   PRICE                DECIMAL(10,2)        null,
   STATUS               orderstatus          default '正常',
   constraint PK_ORDERS primary key (ORDERID)
);
drop table TICKET;

/*==============================================================*/
/* Table: TICKET                                                */
/*==============================================================*/
create table TICKET (
   TICKETID             VARCHAR(20)          not null,
   TICKET_DATE          TIMESTAMP            not null,
   TRAIN                VARCHAR(20)          not null,
   DSNAME               VARCHAR(20)          not null,
   ASNAME               VARCHAR(20)          not null,
   SEATID               INT2                 not null,
   USERNAME             VARCHAR(20)          not null,
   ORDERID              VARCHAR(20)          not null,
   constraint PK_TICKET primary key (TICKETID),
   constraint AK_KEY_2_TICKET unique (TICKET_DATE, TRAIN, DSNAME, ASNAME, SEATID)
);

alter table TICKET
   add constraint FK_TICKET_REFERENCE_ORDERS foreign key (ORDERID)
      references ORDERS (ORDERID)
      on delete restrict on update restrict;

alter table TICKET
   add constraint FK_SOLD_TIC_REFERENCE_TRAIN_ST_D foreign key (TRAIN, DSNAME)
      references TRAIN_STATION (TRAIN, SNAME)
      on delete restrict on update restrict;

alter table TICKET
   add constraint FK_SOLD_TIC_REFERENCE_TRAIN_ST_A foreign key (TRAIN, ASNAME)
      references TRAIN_STATION (TRAIN, SNAME)
      on delete restrict on update restrict;

alter table TICKET
   add constraint FK_TICKET_REFERENCE_SEAT foreign key (SEATID)
      references SEAT (SEATID)
      on delete restrict on update restrict;

alter table TICKET
   add constraint FK_TICKET_REFERENCE_PASSENGE foreign key (USERNAME)
      references PASSENGER (USERNAME)
      on delete restrict on update restrict;



CREATE VIEW TCK_STOPNUM AS 
SELECT TICKETID, TICKET_DATE, TCK.TRAIN AS TRAIN, DSNAME, ASNAME, SEATID, USERNAME, ORDERID,
    TS1.STOPNUM AS STOPNUM1, TS2.STOPNUM AS STOPNUM2
FROM TICKET AS TCK, TRAIN_STATION AS TS1, TRAIN_STATION AS TS2
WHERE
		TS1.TRAIN = TCK.TRAIN AND TS2.TRAIN = TCK.TRAIN
	AND TS1.SNAME = TCK.DSNAME AND TS2.SNAME = TCK.ASNAME
;


