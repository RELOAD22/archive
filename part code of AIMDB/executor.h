#ifndef _EXECUTOR_H
#define _EXECUTOR_H

#include "catalog.h"
#include "mymemory.h"
uint32_t gethash(char *key, BasicType *type);
/** aggrerate method. */
enum AggrerateMethod
{
    NONE_AM = 0, /**< none */
    COUNT,       /**< count of rows */
    SUM,         /**< sum of data */
    AVG,         /**< average of data */
    MAX,         /**< maximum of data */
    MIN,         /**< minimum of data */
    MAX_AM
};

/** compare method. */
enum CompareMethod
{
    NONE_CM = 0,
    LT,   /**< less than */
    LE,   /**< less than or equal to */
    EQ,   /**< equal to */
    NE,   /**< not equal than */
    GT,   /**< greater than */
    GE,   /**< greater than or equal to */
    LINK, /**< join */
    MAX_CM
};

/** definition of request column. */
struct RequestColumn
{
    char name[128];                   /**< name of column */
    AggrerateMethod aggrerate_method; /** aggrerate method, could be NONE_AM  */
};

/** definition of request table. */
struct RequestTable
{
    char name[128]; /** name of table */
};

/** definition of compare condition. */
struct Condition
{
    RequestColumn column;  /**< which column */
    CompareMethod compare; /**< which method */
    char value[128];       /**< the value to compare with, if compare==LINK,value is another column's name; else it's the column's value*/
};

/** definition of conditions. */
struct Conditions
{
    int condition_num;      /**< number of condition in use */
    Condition condition[4]; /**< support maximum 4 & conditions */
};

/** definition of selectquery.  */
class SelectQuery
{
public:
    int64_t database_id;            /**< database to execute */
    int select_number;              /**< number of column to select */
    RequestColumn select_column[4]; /**< columns to select, maximum 4 */
    int from_number;                /**< number of tables to select from */
    RequestTable from_table[4];     /**< tables to select from, maximum 4 */
    Conditions where;               /**< where meets conditions, maximum 4 & conditions */
    int groupby_number;             /**< number of columns to groupby */
    RequestColumn groupby[4];       /**< columns to groupby */
    Conditions having;              /**< groupby conditions */
    int orderby_number;             /**< number of columns to orderby */
    RequestColumn orderby[4];       /**< columns to orderby */
};                                  // class SelectQuery

/** definition of result table.  */
class ResultTable
{
public:
    int column_number;       /**< columns number that a result row consist of */
    BasicType **column_type; /**< each column data type */
    char *buffer;            /**< pointer of buffer alloced from g_memory */
    int64_t buffer_size;     /**< size of buffer, power of 2 */
    int row_length;          /**< length per result row */
    int row_number;          /**< current usage of rows */
    int row_capicity;        /**< maximum capicity of rows according to buffer size and length of row */
    int *offset;
    int offset_size;

    /**
     * init alloc memory and set initial value
     * @col_types array of column type pointers
     * @col_num   number of columns in this ResultTable
     * @param  capicity buffer_size, power of 2
     * @retval >0  success
     * @retval <=0  failure
     */
    int init(BasicType *col_types[], int col_num, int64_t capicity = 1024);
    /**
     * calculate the char pointer of data spcified by row and column id
     * you should set up column_type,then call init function
     * @param row    row id in result table
     * @param column column id in result table
     * @retval !=NULL pointer of a column
     * @retval ==NULL error
     */
    char *getRC(int row, int column);
    /**
     * write data to position row,column
     * @param row    row id in result table
     * @param column column id in result table
     * @data data pointer of a column
     * @retval !=NULL pointer of a column
     * @retval ==NULL error
     */
    int writeRC(int row, int column, void *data);
    /**
     * print result table, split by '\t', output a line per row
     * @retval the number of rows printed
     */
    int print(void);
    /**
     * write to file with FILE *fp
     */
    int dump(FILE *fp);
    /**
     * free memory of this result table to g_memory
     */
    int shut(void);
}; // class ResultTable

/** definition of class Operator */
class Operator
{
public:
    RowTable *table_in[2];                /**< input table (join operator has two tables) */
    RowTable *table_out;                  /**< output table */
    int64_t table_in_col_num[2] = {0, 0}; /**< input table (join operator has two tables) */
    int64_t table_out_col_num = 0;        /**< the number of column in table_out. */
    BasicType **in_col_type;              /**< column types of input tables. */
    int64_t total_row_num = 0;

    virtual bool init() = 0;
    virtual bool getNext(ResultTable *result) = 0;
    virtual bool getNext(ResultTable *result, int64_t row_num) = 0;
    virtual bool reset() = 0;
    virtual bool close() = 0;
};

/** definition of class executor.  */
class Executor
{
private:
    Operator *final_op = NULL;

public:
    /**
     * exec function.
     * @param  query to execute, if NULL, execute query at last time
     * @param result table generated by an execution, store result in pattern defined by the result table
     * @retval >0  number of result rows stored in result
     * @retval <=0 no more result
     */
    virtual int exec(SelectQuery *query, ResultTable *result);
    /**
     * close function.
     * @param None
     * @retval ==0 succeed to close
     * @retval !=0 fail to close
     */
    virtual int close();
};
/** definition of class executor.  */
class Scan : public Operator
{
private:
    int64_t current_row; /**< row num to be scaned. begin 0 */
    int64_t total_row;   /**<total row num */
public:
    /**
     * construction of Scan
     * @param Op prior oprators
     * @param join condtions
     */
    Scan(char *tablename);
    /**
     * init scan.
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of Scan
     * @param result store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of scan
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset();
    /**
     * close scan
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
};

/** definition of class Filter.  */
class Filter : public Operator
{
private:
    Operator *prior_op;    /**< operator of prior operation*/
    int64_t col_rank;      /**< the rank of column*/
    char value[1024];      /**< value*/
    BasicType *value_type; /**< column type*/
    CompareMethod compare; /**< compare method*/
public:
    /**
     * construction of Filter
     * @param Op prior oprators
     * @param join condtions
     */
    Filter(Operator *Op, Condition *condi);
    /**
     * init filter
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of filter
     * @param result buffer to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of filter
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset();
    /**
     * close filter
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
    /**
     * compare left and right
     * @retval false for failure
     * @retval true  for success
     */
    bool filter_compare(char *cmp_left_ptr, char *cmp_right_ptr)
    {
        if (this->compare == LT)
            return this->value_type->cmpLT(cmp_left_ptr, cmp_right_ptr);
        else if (this->compare == LE)
            return this->value_type->cmpLE(cmp_left_ptr, cmp_right_ptr);
        else if (this->compare == EQ)
            return this->value_type->cmpEQ(cmp_left_ptr, cmp_right_ptr);
        else if (this->compare == NE)
            return !this->value_type->cmpEQ(cmp_left_ptr, cmp_right_ptr);
        else if (this->compare == GT)
            return this->value_type->cmpGT(cmp_left_ptr, cmp_right_ptr);
        else if (this->compare == GE)
            return this->value_type->cmpGE(cmp_left_ptr, cmp_right_ptr);
        else
            return false;
    }
};

/** definition of class project*/
class Project : public Operator
{
private:
    Operator *prior_op;  /**< prior operator */
    int64_t col_rank[4]; /**< rank sets of columns projected*/
public:
    /**
     * construction of Project
     * @param Op prior oprators
     * @param join condtions
     */
    Project(Operator *Op, int64_t col_tot, RequestColumn *cols_name);
    /**
     * init Project
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of Project
     * @param result buffer to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of Project
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset() { return true; };
    /**
     * close Project
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
};

/** definition of class hashjoin. */
class HashJoin : public Operator
{
private:
    Operator *Op[2] = {NULL, NULL}; /**< prior operators*/
    int64_t lef_column_rank = -1;   /**< left column rank*/
    int64_t right_column_rank = -1; /**< right column rank */
    ResultTable tmp_resulttables;   /**< result table to store temp result*/
    HashTable *hash_table = NULL;   /**< hash table*/
    ResultTable result;             /**< tmp result*/
    bool hashjoin_finish = true;    /**< one round finish or not*/
    int hash_pos_now = 0;           /**< position in one round*/
public:
    /**
     * construction of HashJoin
     * @param Op prior oprators
     * @param join condtions
     */
    HashJoin(Operator **Op, Condition *condi);
    /**
     * init hashjoin.
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of hashjoin
     * @param result buffer to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of hashjoin
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset();
    /**
     * close operator
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
};

/** definition of GroupBy */
class GroupBy : public Operator
{
private:
    Operator *prior_op;                  /**< prior operators*/
    int group_aggnum = 0;                /**< number of group aggrerate */
    int group_keynum = 0;                /**< number of group key*/
    BasicType *aggrerate_type[4];        /**< type of group aggrerate*/
    BasicType *non_aggrerate_type[4];    /**< type of group key*/
    size_t aggrerate_off[4];             /**< aggrerate offset*/
    size_t non_aggrerate_off[4];         /**< key offset*/
    AggrerateMethod aggrerate_method[4]; /**< aggrerate methods*/
    ResultTable tmp_result;              /**< tmp result*/
    int current_row = 0;                 /**< row num to be pick. begin 0 */
public:
    /**
     * construction of GroupBy
     * @param Op prior oprators
     * @param groupby_num the number of group by cols
     * @param req_col columns of group by
     */
    GroupBy(Operator *Op, int groupby_num, RequestColumn *req_col);
    /**
     * init GroupBy.
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of GroupBy
     * @param result buffer to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of GroupBy
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset() { return true; };
    /**
     * close operator
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
    /**
     * aggrerate on one row
     */
    bool aggrerate(AggrerateMethod method, int agg_i, void *new_data, void *agg_data);
    /**
     * sum aggrerate
     */
    void sum(int agg_index, void *new_data, void *agg_data)
    {
        switch (aggrerate_type[agg_index]->getTypeCode())
        {
        case INT8_TC:
        {
            int8_t agg_result = *(int8_t *)new_data + *(int8_t *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        case INT16_TC:
        {
            int16_t agg_result = *(int16_t *)new_data + *(int16_t *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        case INT32_TC:
        {
            int32_t agg_result = *(int32_t *)new_data + *(int32_t *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        case INT64_TC:
        {
            int64_t agg_result = *(int64_t *)new_data + *(int64_t *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        case FLOAT32_TC:
        {
            float agg_result = *(float *)new_data + *(float *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        case FLOAT64_TC:
        {
            double agg_result = *(double *)new_data + *(double *)agg_data;
            this->aggrerate_type[agg_index]->copy(agg_data, &agg_result);
            break;
        }
        default:
            break;
        }
    }
    /**
     * avg aggrerate
     */
    void avg(int agg_index, void *new_data, void *agg_data)
    {
        sum(agg_index, new_data, agg_data);
    }
    /**
     * count aggrerate
     */
    void count(int agg_index, void *new_data, void *agg_data)
    {
    }
    /**
     * max aggrerate
     */
    void max(int agg_index, void *new_data, void *agg_data)
    {
        if (this->aggrerate_type[agg_index]->cmpLT(agg_data, new_data))
        {
            this->aggrerate_type[agg_index]->copy(agg_data, new_data);
        }
    }
    /**
     * min aggrerate
     */
    void min(int agg_index, void *new_data, void *agg_data)
    {
        if (this->aggrerate_type[agg_index]->cmpLT(new_data, agg_data))
            this->aggrerate_type[agg_index]->copy(agg_data, new_data);
    }
};
/** definiton of OrderBy   */
class OrderBy : public Operator
{
private:
    Operator *prior_op;             /**< prior operators*/
    int64_t orderby_num;            /**< number of order conditions */
    BasicType *compare_col_type[4]; /**< column types of compare conditons    */
    int64_t compare_col_rank[4];    /**< ranks of compare condtions      */
    int64_t compare_col_offset[4];  /**< offset of compared columns      */
    int64_t current_row = 0;        /**< row num to be pick. begin 0 */
    ResultTable tmp_result;         /**< tmp result*/
    int64_t row_length;             /**< the length of each row*/
    char *result_buffer;            /**< use to get row ptr*/
public:
    /**
     * construction of OrderBy
     * @param Op prior oprators
     * @param orderby_num the number of order by cols
     * @param cols_name columns of order by
     */
    OrderBy(Operator *Op, int64_t orderby_num, RequestColumn *cols_name);
    /**
     * init OrderBy.
     * @retval false for failure
     * @retval true  for success
     */
    bool init();
    /**
     * get next record of OrderBy
     * @param result buffer to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result);
    /**
     * get next record of OrderBy
     * @param result buffer to store result
     * @param row_num the row num to store result
     * @retval false for failure
     * @retval true  for success
     */
    bool getNext(ResultTable *result, int64_t row_num);
    /**
     * reset operator
     * @retval false for failure
     * @retval true  for success
     */
    bool reset() { return true; }
    /**
     * close operator
     * @retval false for failure
     * @retval true  for success
     */
    bool close();
    /**
     * get the ptr of row by row_num
     */
    char *get_rowptr(int row_num)
    {
        return result_buffer + row_num * row_length;
    }
    /**
     * quick sort
     * @param buffer the begin of buffer
     * @param left low
     * @param right high
     */
    void quick_sort(char *buffer, int64_t left, int64_t right);
    /**
     * compare
     * @param left_ptr to be compared
     * @param right_ptr to be compared
     * @retval false for <
     * @retval true for >=
     */
    bool GE(void *left_ptr, void *right_ptr);
    /**
     * partition of quick sort
     */
    int partition(char *buffer, int64_t left, int64_t right);
};
#endif