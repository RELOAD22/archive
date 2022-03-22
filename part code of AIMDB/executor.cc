#include "executor.h"
#include <string>
#include <math.h>
using namespace std;
int64_t tabout_id_hashjoin = 777;

uint32_t gethash(const char *key)
{
    uint32_t hash = 0;
    for (unsigned i = 0; i < strlen(key); i++)
        hash = 31 * hash + key[i];
    return hash;
}
int64_t numTo2N(int64_t x)
{
    int i = 0;
    double tmp = x;
    while (tmp >= 2)
    {
        i++;
        tmp /= 2.0;
    }
    return pow(2, (tmp > 1 ? i + 1 : i));
}
int64_t filter_tableid[4] = {0, 0, 0, 0};
//对基本的where条件找到对应的table
void find_table_ofwhere(SelectQuery *query)
{
    for (int i = 0; i < query->where.condition_num; i++)
    {
        //非连接的where条件
        if (query->where.condition[i].compare != LINK)
        {
            Column *filter_column = (Column *)g_catalog.getObjByName(query->where.condition[i].column.name);
            int64_t filter_columnid = filter_column->getOid();

            //对每个表的所有列遍历，寻找匹配，放入filter_tableid
            for (int j = 0; j < query->from_number; j++)
            {
                Table *table = (Table *)g_catalog.getObjByName(query->from_table[j].name);
                auto tablecolumns = table->getColumns();
                for (int num = 0; num < (int)tablecolumns.size(); num++)
                {
                    if (filter_columnid == tablecolumns[num])
                    {
                        filter_tableid[i] = table->getOid();
                        break;
                    }
                }
            }
        }
    }
}

Column *get_column_from_name(char *name)
{
}
int64_t get_columnid_from_name(char *name)
{
    return ((Column *)g_catalog.getObjByName(name))->getOid();
}

int64_t join_left_tableid[4] = {0, 0, 0, 0};
int64_t join_right_tableid[4] = {0, 0, 0, 0};
int64_t join_count = 0;
Condition *join_conditions[4];
int64_t row_count = 0;
//对连接的where条件找到对应的table
void find_table_ofjoin(SelectQuery *query)
{
    for (int i = 0; i < query->where.condition_num; i++)
    {
        //连接的where条件
        if (query->where.condition[i].compare == LINK)
        {
            join_conditions[join_count] = &(query->where.condition[i]);
            int64_t join_left_columnid = get_columnid_from_name(query->where.condition[i].column.name);
            int64_t join_right_columnid = get_columnid_from_name(query->where.condition[i].value);
            for (int j = 0; j < query->from_number; j++)
            {
                Table *table = (Table *)g_catalog.getObjByName(query->from_table[j].name);
                auto columns = table->getColumns();
                for (int k = 0; k < (int)columns.size(); k++)
                {
                    if (join_left_columnid == columns[k])
                    {
                        join_left_tableid[join_count] = j;
                    }
                    if (join_right_columnid == columns[k])
                    {
                        join_right_tableid[join_count] = j;
                    }
                }
            }
            ++join_count;
        }
    }
}

int Executor::exec(SelectQuery *query, ResultTable *result)
{
    if (query != NULL)
    {
        //第一次执行query，初始化各项内容，并建树
        find_table_ofwhere(query);
        find_table_ofjoin(query);
        //--------------------scan-------------filter--------------
        Operator *filterscan_array[4];
        for (int i = 0; i < query->from_number; ++i)
        {
            RowTable *row_table = (RowTable *)g_catalog.getObjByName(query->from_table[i].name);
            filterscan_array[i] = new Scan(query->from_table[i].name);
            int64_t row_tid = row_table->getOid();
            printf("scan %s on tableid:%d\n", query->from_table[i].name, row_tid);
            for (int j = 0; j < 4; j++)
            {
                if (filter_tableid[j] == row_tid)
                {
                    filterscan_array[i] = new Filter(filterscan_array[i], &query->where.condition[j]);
                    printf("filter %d on scan %s\n", j, query->from_table[i].name);
                }
            }
        }
        printf("finish filter scan\n");
        //-------------------join--------------
        Operator *op_on_onetable;
        if (join_count > 1)
        {
            Operator ***hash_op = new Operator **[4];
            for (int i = 0; i < join_count; i++)
            {
                printf("join: left-tableid:%d right-tableid:%d \n", join_left_tableid[i], join_right_tableid[i]);
                hash_op[i] = new Operator *[2];
                hash_op[i][0] = filterscan_array[join_left_tableid[i]];
                hash_op[i][1] = filterscan_array[join_right_tableid[i]];
                op_on_onetable = new HashJoin(hash_op[i], join_conditions[i]);
                filterscan_array[join_left_tableid[i]] = op_on_onetable;
                filterscan_array[join_right_tableid[i]] = op_on_onetable;
            }
        }
        else if (join_count == 1)
        {
            printf("join: left-tableid:%d right-tableid:%d \n", join_left_tableid[0], join_right_tableid[0]);
            op_on_onetable = new HashJoin(filterscan_array, join_conditions[0]);
        }
        else
            op_on_onetable = filterscan_array[0];
        printf("finish JOIN\n");

        //-------------------project--------------
        if (query->select_number > 0)
            op_on_onetable = new Project(op_on_onetable, query->select_number, query->select_column);
        printf("finish PROJECT\n");

        //-------------------GROUPBY--------------
        if (query->groupby_number > 0)
            op_on_onetable = new GroupBy(op_on_onetable, query->select_number, query->select_column);
        printf("finish GROUPBY\n");
        //-------------------ORDERBY--------------
        if (query->orderby_number > 0)
            op_on_onetable = new OrderBy(op_on_onetable, query->orderby_number, query->orderby);
        printf("finish ORDERBY\n");

        //-------------------having--------------
        for (int i = 0; i < query->having.condition_num; ++i)
        {
            op_on_onetable = new Filter(op_on_onetable, &query->having.condition[i]);
            printf("having %d\n", i);
        }
        printf("finish having\n");

        // =========================init============================
        op_on_onetable->init();
        printf("finish INIT\n");
        this->final_op = op_on_onetable;
    }

    // ===================build result table=====================
    int fcol_num = (int)final_op->table_out->getColumns().size();
    auto frow_pattern = final_op->table_out->getRPattern();
    auto fresult_type = new BasicType *[fcol_num];
    for (int i = 0; i < fcol_num; i++)
        fresult_type[i] = frow_pattern.getColumnType(i);
    result->init(fresult_type, fcol_num, 2048);
    // ===============get final result====================
    int stat = final_op->getNext(result);

    if (stat > 0)
    {
        ++row_count;
        return 1;
    }
    else
    {
        printf("total row:%d\n", row_count);
        return 0;
    }
}

int Executor::close()
{
    for (int i = 0; i < 4; ++i)
    {
        filter_tableid[i] = 0;
        join_left_tableid[i] = 0;
        join_right_tableid[i] = 0;
    }
    join_count = 0;
    row_count = 0;
    final_op->close();
    g_memory.print();

    printf("finish exec\n");
    return 0;
}

// note: you should guarantee that col_types is useable as long as this ResultTable in use, mayb
int ResultTable::init(BasicType *col_types[], int col_num, int64_t capicity)
{
    column_type = col_types;
    column_number = col_num;
    row_length = 0;
    buffer_size = g_memory.alloc(buffer, capicity);
    if (buffer_size != capicity)
    {
        printf("[ResultTable][ERROR][init]: buffer allocate error!\n");
        return -1;
    }
    int allocate_size = 8;
    int require_size = sizeof(int) * column_number;
    while (allocate_size < require_size)
        allocate_size = allocate_size << 1;
    char *p = NULL;
    offset_size = g_memory.alloc(p, allocate_size);
    if (offset_size != allocate_size)
    {
        printf("[ResultTable][ERROR][init]: offset allocate error!\n");
        return -2;
    }
    offset = (int *)p;
    for (int ii = 0; ii < column_number; ii++)
    {
        offset[ii] = row_length;
        row_length += column_type[ii]->getTypeSize();
    }
    row_capicity = (int)(capicity / row_length);
    row_number = 0;
    return 0;
}

int ResultTable::print(void)
{
    int row = 0;
    int ii = 0;
    char buffer[1024];
    char *p = NULL;
    while (row < row_number)
    {
        for (; ii < column_number - 1; ii++)
        {
            p = getRC(row, ii);
            column_type[ii]->formatTxt(buffer, p);
            printf("%s\t", buffer);
        }
        p = getRC(row, ii);
        column_type[ii]->formatTxt(buffer, p);
        printf("%s\n", buffer);
        row++;
        ii = 0;
    }
    return row;
}

int ResultTable::dump(FILE *fp)
{
    // write to file
    int row = 0;
    int ii = 0;
    char buffer[1024];
    char *p = NULL;
    while (row < row_number)
    {
        for (; ii < column_number - 1; ii++)
        {
            p = getRC(row, ii);
            column_type[ii]->formatTxt(buffer, p);
            fprintf(fp, "%s\t", buffer);
        }
        p = getRC(row, ii);
        column_type[ii]->formatTxt(buffer, p);
        fprintf(fp, "%s\n", buffer);
        row++;
        ii = 0;
    }
    return row;
}

// this include checks, may decrease its speed
char *ResultTable::getRC(int row, int column)
{
    return buffer + row * row_length + offset[column];
}

int ResultTable::writeRC(int row, int column, void *data)
{
    char *p = getRC(row, column);
    if (p == NULL)
        return 0;
    // add here
    if (row + 1 > row_number)
    {
        row_number = row + 1;
    }
    return column_type[column]->copy(p, data);
}

int ResultTable::shut(void)
{
    // free memory
    g_memory.free(buffer, buffer_size);
    g_memory.free((char *)offset, offset_size);
    return 0;
}

//---------------------operators implementation---------------------------
Scan::Scan(char *tablename)
{
    RowTable *table = (RowTable *)g_catalog.getObjByName(tablename);
    this->table_in[0] = table;
    this->table_out = table;
    this->table_in_col_num[0] = table->getColumns().size();
    this->table_out_col_num = table->getColumns().size();
    printf("table:%s col_num:%d\n", tablename, this->table_in_col_num[0]);
    this->total_row = table->getRecordNum();
    this->total_row_num = table->getRecordNum();
}
bool Scan::init(void)
{
    //初始化扫描行数
    this->current_row = 0;
    return true;
}
bool Scan::getNext(ResultTable *result)
{
    return getNext(result, 0);
    ;
}
bool Scan::getNext(ResultTable *result, int64_t row_num)
{
    //从当前扫描位置再向下读出一行记录
    // printf("scanget1:%d resultcap%d\n",row_num,result->row_capicity);
    if (this->current_row == total_row)
        return false;

    char buffer[1024];
    for (int current_colnum = 0; current_colnum < this->table_in_col_num[0]; current_colnum++)
    {
        this->table_in[0]->selectCol(current_row, current_colnum, buffer);
        result->writeRC(row_num, current_colnum, buffer);
    }
    // printf("scangetf\n");
    this->current_row++;
    return true;
}
bool Scan::reset()
{
    current_row = 0;
    return true;
}
bool Scan::close()
{
    return true;
}

Filter::Filter(Operator *Op, Condition *condi)
{
    this->prior_op = Op;
    this->compare = condi->compare;
    this->table_in[0] = this->prior_op->table_out;
    this->table_out = this->table_in[0];
    this->table_out_col_num = this->table_out->getColumns().size();

    int64_t columnid = get_columnid_from_name(condi->column.name);
    this->col_rank = this->table_out->getColumnRank(columnid);
    this->value_type = this->table_out->getRPattern().getColumnType(this->col_rank);
    this->value_type->formatBin(this->value, condi->value);
}
bool Filter::init()
{
    prior_op->init();
    this->total_row_num = this->prior_op->total_row_num;
    return true;
}
bool Filter::getNext(ResultTable *result)
{
    return getNext(result, 0);
}
bool Filter::getNext(ResultTable *result, int64_t row_num)
{
    char *cmpSrcA_ptr = NULL;
    char *cmpSrcB_ptr = NULL;
    do
    {
        if (!prior_op->getNext(result, row_num))
        {
            return false;
        }
        cmpSrcA_ptr = result->getRC(row_num, this->col_rank);
        cmpSrcB_ptr = this->value;
    } while (!(this->filter_compare(cmpSrcA_ptr, cmpSrcB_ptr)));
    return true;
}
bool Filter::reset()
{
    return prior_op->reset();
}
bool Filter::close()
{
    return prior_op->close();
}

Project::Project(Operator *Op, int64_t col_tot, RequestColumn *cols_name)
{
    this->prior_op = Op;
    table_in_col_num[0] = Op->table_out->getColumns().size();
    this->table_out_col_num = col_tot;
    this->table_in[0] = Op->table_out;

    in_col_type = new BasicType *[table_in_col_num[0]];
    for (int i = 0; i < table_in_col_num[0]; i++)
        in_col_type[i] = this->prior_op->table_out->getRPattern().getColumnType(i);

    for (int i = 0; i < col_tot; i++)
    {
        int64_t columnid = get_columnid_from_name(cols_name[i].name);
        this->col_rank[i] = Op->table_out->getColumnRank(columnid);
    }

    const char *ctablename = "project_table";
    size_t len = strlen(ctablename);
    char *tablename = new char[len + 1];
    memcpy(tablename, ctablename, len + 1);

    RowTable *project_table = (RowTable *)g_catalog.getObjByName(tablename);
    table_out = new RowTable(765, tablename);
    table_out->init();
    RPattern *new_RPattern = &this->table_out->getRPattern();
    RPattern *old_RPattern = &Op->table_out->getRPattern();
    auto &cols_id = table_in[0]->getColumns();
    new_RPattern->init(col_tot);

    for (int i = 0; i < col_tot; i++)
    {
        // printf("%d\n",col_rank[i]);
        new_RPattern->addColumn(old_RPattern->getColumnType(col_rank[i]));
        table_out->addColumn(cols_id[col_rank[i]]);
    }
}
bool Project::init()
{
    prior_op->init();
    this->total_row_num = this->prior_op->total_row_num;
    return true;
}
bool Project::getNext(ResultTable *result)
{
    return getNext(result, 0);
}
bool Project::getNext(ResultTable *result, int64_t row_num)
{
    ResultTable tmp_result;
    tmp_result.init(in_col_type, table_in_col_num[0]);
    if (!this->prior_op->getNext(&tmp_result))
    {
        tmp_result.shut();
        return false;
    }
    for (int i = 0; i < this->table_out_col_num; i++)
    {
        char *tmp = tmp_result.getRC(0, col_rank[i]);
        result->writeRC(row_num, i, tmp);
    }
    tmp_result.shut();
    return true;
}
bool Project::close()
{
    return prior_op->close();
}

HashJoin::HashJoin(Operator **Op, Condition *condi)
{
    this->Op[0] = Op[0];
    table_in[0] = Op[0]->table_out;
    table_in_col_num[0] = (int)table_in[0]->getColumns().size();
    this->Op[1] = Op[1];
    table_in[1] = Op[1]->table_out;
    table_in_col_num[1] = (int)table_in[1]->getColumns().size();
    table_out_col_num = table_in_col_num[0] + table_in_col_num[1];
    int64_t left_colunmid = get_columnid_from_name(condi->column.name);
    int64_t right_colunmid = get_columnid_from_name(condi->value);

    // table_in[0] --- left_colunmid
    // table_in[1] --- right_colunmid
    bool flag = true;
    auto &cols_id = table_in[0]->getColumns();
    for (int i = 0; i < table_in_col_num[0]; i++)
        if (cols_id[i] == left_colunmid)
        {
            flag = false;
            break;
        }
    if (flag)
    {
        auto tmp = left_colunmid;
        left_colunmid = right_colunmid;
        right_colunmid = tmp;
    }
    lef_column_rank = table_in[0]->getColumnRank(left_colunmid);
    right_column_rank = table_in[1]->getColumnRank(right_colunmid);

    const char *ctablename = ("project_table" + to_string(tabout_id_hashjoin)).c_str();
    size_t len = strlen(ctablename);
    char *tablename = new char[len + 1];
    memcpy(tablename, ctablename, len + 1);
    RowTable *hashjoin_table = (RowTable *)g_catalog.getObjByName(tablename);
    table_out = new RowTable(tabout_id_hashjoin++, tablename);
    table_out->init();

    RPattern *new_RPattern = &table_out->getRPattern();
    RPattern *old_RPattern = NULL;

    new_RPattern->init(table_out_col_num);

    in_col_type = new BasicType *[table_out_col_num];
    BasicType *t;
    int cnt = 0;
    for (int i = 0; i < 2; i++)
    {
        old_RPattern = &table_in[i]->getRPattern();
        auto &cols_id = table_in[i]->getColumns();
        for (int j = 0; j < table_in_col_num[i]; j++)
        {
            in_col_type[cnt++] = old_RPattern->getColumnType(j);
            t = old_RPattern->getColumnType(table_in[i]->getColumnRank(cols_id[j]));
            new_RPattern->addColumn(t);
            table_out->addColumn(cols_id[j]);
        }
    }
    this->result.init(in_col_type, table_in_col_num[0]);
}
bool HashJoin::init(void)
{
    Op[0]->init();
    Op[1]->init();
    BasicType **right_columntype = new BasicType *[table_in_col_num[1]];
    for (int i = 0; i < table_in_col_num[1]; i++)
        right_columntype[i] = table_in[1]->getRPattern().getColumnType(i);
    char *value;
    char format_value[1024];
    hash_table = new HashTable(60000, 10, 0);
    int right_columnrow = 0;
    printf("hash init\n");
    total_row_num = 0;
    ResultTable tmp_justforlength;
    tmp_justforlength.init(right_columntype, table_in_col_num[1]);
    while (Op[1]->getNext(&tmp_justforlength))
    {
        ++total_row_num;
        // printf("count row:%d\n",total_row_num);
    }
    Op[1]->reset();
    // printf("count finish\n");

    int64_t capacity = numTo2N(tmp_justforlength.row_length * total_row_num);
    printf("row_length%d total_row_num%d capacity%d\n", tmp_justforlength.row_length, total_row_num, capacity);
    // tmp_resulttables.init(right_columntype, table_in_col_num[1], capacity);
    tmp_resulttables.init(right_columntype, table_in_col_num[1], 1 << 28);
    printf("alloc for row_num:%d\n", (1 << 28) / tmp_justforlength.row_length);
    while (Op[1]->getNext(&tmp_resulttables, right_columnrow))
    {
        value = tmp_resulttables.getRC(right_columnrow, right_column_rank);
        BasicType *tmp_type = table_in[1]->getRPattern().getColumnType(right_column_rank);
        tmp_type->formatTxt(format_value, value);
        uint32_t hash_value = gethash(format_value);
        hash_table->add(hash_value, (char *)right_columnrow);
        right_columnrow++;
        // printf("row:%d\n",right_columnrow);
        // tmp_resulttables[right_columnrow].init(right_columntype, table_in_col_num[1]);
    }
    printf("%d hash init finish\n", right_columnrow);
    tmp_justforlength.shut();
    return true;
}
bool HashJoin::getNext(ResultTable *result)
{
    return getNext(result, 0);
}
bool HashJoin::getNext(ResultTable *result, int64_t row_num)
{
    bool flag = false;
    char *cmpSrcA_ptr;
    char *cmpSrcB_ptr;
    // ResultTable * tmp_resulttables;
    int right_columnrow = 0;
    while (!flag)
    {
        if (hashjoin_finish == true)
        {
            //完成一轮
            if (!Op[0]->getNext(&this->result))
            {
                printf("join match finish\n");
                break;
            }
            hashjoin_finish = false;
        }
        //未完成一轮
        cmpSrcA_ptr = this->result.getRC(0, lef_column_rank);

        BasicType *value_type = table_in[0]->getRPattern().getColumnType(lef_column_rank);
        char *hashjoin_format_value = new char[128];
        value_type->formatTxt(hashjoin_format_value, cmpSrcA_ptr);

        uint32_t hash_result = gethash(hashjoin_format_value);
        int64_t init_cap = 10000;
        char **hashjoin_cmpSrcB_tables = new char *[init_cap];
        int64_t cmpSrcB_table_num;
        while ((cmpSrcB_table_num = hash_table->probe(hash_result, hashjoin_cmpSrcB_tables, init_cap)) < 0)
        {
            delete hashjoin_cmpSrcB_tables;
            init_cap *= 10;
            hashjoin_cmpSrcB_tables = new char *[init_cap];
        }

        for (int i = hash_pos_now; i < cmpSrcB_table_num; i++, hash_pos_now++)
        {
            right_columnrow = (int &)(hashjoin_cmpSrcB_tables[i]);
            // printf("row:%d\n", right_columnrow);
            cmpSrcB_ptr = tmp_resulttables.getRC(right_columnrow, right_column_rank);
            if (value_type->cmpEQ(cmpSrcA_ptr, cmpSrcB_ptr))
            {
                flag = true;
                hash_pos_now++;
                break;
            }
        }
        delete hashjoin_cmpSrcB_tables;
        if (hash_pos_now == cmpSrcB_table_num || cmpSrcB_table_num <= 0)
        {
            hashjoin_finish = true;
            hash_pos_now = 0;
        }
        if (flag)
            break;
    }

    if (flag)
    {
        char *buffer;
        for (int j = 0; j < table_in_col_num[0]; j++)
        {
            buffer = this->result.getRC(0, j);
            result->writeRC(row_num, j, buffer);
        }
        for (int j = 0; j < table_in_col_num[1]; j++)
        {
            buffer = tmp_resulttables.getRC(right_columnrow, j);
            result->writeRC(row_num, table_in_col_num[0] + j, buffer);
        }
    }
    return flag;
}
bool HashJoin::reset()
{
    if (!Op[0]->reset())
        return false;
    if (!Op[1]->reset())
        return false;
    hashjoin_finish = true;
    hash_pos_now = 0;
    return true;
}
bool HashJoin::close(void)
{
    if (!Op[0]->close())
        return false;
    if (!Op[1]->close())
        return false;
    delete hash_table;
    tmp_resulttables.shut();
    printf("hashjoin close\n");
    return true;
}

GroupBy::GroupBy(Operator *Op, int selectnum, RequestColumn req_col[4])
{
    this->prior_op = Op;
    this->table_in[0] = this->prior_op->table_out;
    this->table_out = this->prior_op->table_out;
    this->table_in_col_num[0] = this->table_out->getColumns().size();
    this->table_out_col_num = this->table_out->getColumns().size();

    for (int i = 0; i < selectnum; i++)
    {
        if (req_col[i].aggrerate_method == NONE_AM)
        {
            this->non_aggrerate_off[this->group_keynum] = i;
            this->non_aggrerate_type[this->group_keynum] = table_out->getRPattern().getColumnType(i);
            this->group_keynum++;
        }
        else
        {
            this->aggrerate_off[this->group_aggnum] = i;
            this->aggrerate_type[this->group_aggnum] = table_out->getRPattern().getColumnType(i);
            this->aggrerate_method[this->group_aggnum] = req_col[i].aggrerate_method;
            this->group_aggnum++;
        }
    }
}

bool GroupBy::init()
{
    prior_op->init();
    this->prior_op->total_row_num;

    RPattern in_RP = table_in[0]->getRPattern();
    int in_colnum = table_in[0]->getColumns().size();
    BasicType **in_col_type;
    in_col_type = new BasicType *[in_colnum];
    for (int i = 0; i < in_colnum; i++)
    {
        in_col_type[i] = in_RP.getColumnType(i);
    }
    ResultTable tmp_one;
    tmp_one.init(in_col_type, in_colnum);
    int64_t row_size = this->table_out->getRPattern().getRowSize();
    int64_t capacity = numTo2N(row_size * this->prior_op->total_row_num);
    printf("row_length%d total_row_num%d capacity%d\n", row_size, this->prior_op->total_row_num, capacity);
    // tmp_result.init(in_col_type, in_colnum, capacity);
    tmp_result.init(in_col_type, in_colnum, 1 << 27);

    HashTable *hstable = new HashTable(100000, 2, 0);
    int64_t hash_count = 0;

    char **probe_result = (char **)malloc(4 * numTo2N(row_size));
    char tmp_buffer[1024];
    int64_t *pattern_count = (int64_t *)malloc(sizeof(int64_t) * this->prior_op->total_row_num);
    for (int i = 0; i < this->prior_op->total_row_num; i++)
        pattern_count[i] = 0;
    while (prior_op->getNext(&tmp_one))
    {
        string s_buffer = " ";
        for (int i = 0; i < group_keynum; i++)
        {
            char *src = tmp_one.getRC(0, non_aggrerate_off[i]);
            tmp_one.column_type[non_aggrerate_off[i]]->formatTxt(tmp_buffer, src);
            s_buffer += " ";
            s_buffer += tmp_buffer;
        }

        int64_t key = gethash(s_buffer.data());
        if (hstable->probe(key, probe_result, 10) > 0)
        {
            //找到了，执行聚集策略
            pattern_count[(uint64_t)probe_result[0]]++;
            for (int i = 0; i < group_aggnum; i++)
            {
                char *new_data = tmp_one.getRC(0, aggrerate_off[i]);
                char *agg_data = tmp_result.getRC((uint64_t)probe_result[0], aggrerate_off[i]);
                aggrerate(aggrerate_method[i], i, new_data, agg_data);
            }
        }
        else
        {
            //未找到，在hash中添加
            hstable->add(key, (char *)hash_count);
            for (int j = 0; j < table_in_col_num[0]; j++)
            {
                char *src = tmp_one.getRC(0, j);
                tmp_result.writeRC(hash_count, j, src);
            }
            pattern_count[hash_count] += 1;
            hash_count++;
        }
    }
    for (int j = 0; j < hash_count; j++)
    {
        for (int k = 0; k < group_aggnum; k++)
        {
            auto new_data = tmp_result.getRC(j, aggrerate_off[k]);
            if (this->aggrerate_method[k] == COUNT)
            {
                switch (aggrerate_type[k]->getTypeCode())
                {
                case INT8_TC:
                {
                    int8_t agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT16_TC:
                {
                    int16_t agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT32_TC:
                {
                    int32_t agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT64_TC:
                {
                    int64_t agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case FLOAT32_TC:
                {
                    float agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case FLOAT64_TC:
                {
                    double agg_result = pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                default:
                    break;
                }
            }
            else if (this->aggrerate_method[k] == AVG)
            {
                switch (aggrerate_type[k]->getTypeCode())
                {
                case INT8_TC:
                {
                    int8_t agg_result = (*(int8_t *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT16_TC:
                {
                    int16_t agg_result = (*(int16_t *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT32_TC:
                {
                    int32_t agg_result = (*(int32_t *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case INT64_TC:
                {
                    int64_t agg_result = (*(int64_t *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case FLOAT32_TC:
                {
                    float agg_result = (*(float *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                case FLOAT64_TC:
                {
                    double agg_result = (*(double *)new_data) / pattern_count[j];
                    this->aggrerate_type[k]->copy(new_data, &agg_result);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
    this->total_row_num = hash_count;
    free(probe_result);
    tmp_one.shut();
    free(pattern_count);
    delete hstable;
    return true;
}
bool GroupBy::getNext(ResultTable *result)
{
    return getNext(result, 0);
}
bool GroupBy::getNext(ResultTable *result, int64_t row_num)
{
    if (this->current_row >= tmp_result.row_number)
        return false;
    for (int j = 0; j < table_out_col_num; j++)
    {
        char *buf = tmp_result.getRC(this->current_row, j);
        result->writeRC(row_num, j, buf);
    }
    current_row++;
    return true;
}
bool GroupBy::close()
{
    tmp_result.shut();
    printf("GroupBy close\n");
    return prior_op->close();
}
bool GroupBy::aggrerate(AggrerateMethod method, int agg_index, void *new_data, void *agg_data)
{
    switch (method)
    {
    case SUM:
        this->sum(agg_index, new_data, agg_data);
        break;
    case AVG:
        this->avg(agg_index, new_data, agg_data);
        break;
    case COUNT:
        this->count(agg_index, new_data, agg_data);
        break;
    case MAX:
        this->max(agg_index, new_data, agg_data);
        break;
    case MIN:
        this->min(agg_index, new_data, agg_data);
        break;
    default:
        exit(0);
        break;
    }
}

OrderBy::OrderBy(Operator *Op, int64_t orderby_num, RequestColumn *cols_name)
{
    this->prior_op = Op;
    this->table_in[0] = this->prior_op->table_out;
    this->table_out = this->prior_op->table_out;
    this->table_out_col_num = this->table_out->getColumns().size();
    this->table_in_col_num[0] = table_out_col_num;
    this->orderby_num = orderby_num;

    in_col_type = new BasicType *[this->table_out_col_num];
    for (int i = 0; i < this->table_out_col_num; i++)
    {
        this->in_col_type[i] = this->table_out->getRPattern().getColumnType(i);
    }

    this->row_length = this->table_out->getRPattern().getRowSize();
    for (int i = 0; i < this->orderby_num; i++)
    {
        Object *col_ptr = g_catalog.getObjByName(cols_name[i].name);
        this->compare_col_rank[i] = this->prior_op->table_out->getColumnRank(col_ptr->getOid());
        this->compare_col_type[i] = this->table_out->getRPattern().getColumnType(compare_col_rank[i]);
        this->compare_col_offset[i] = this->table_out->getRPattern().getColumnOffset(compare_col_rank[i]);
    }
}
bool OrderBy::init()
{
    this->prior_op->init();

    int64_t row_size = this->table_out->getRPattern().getRowSize();
    int64_t capacity = numTo2N(row_size * this->prior_op->total_row_num);
    printf("row_length%d total_row_num%d capacity%d\n", row_size, this->prior_op->total_row_num, capacity);
    // this->tmp_result.init(in_col_type, table_out_col_num, capacity);
    this->tmp_result.init(in_col_type, table_out_col_num, 1 << 27);

    this->result_buffer = tmp_result.buffer;

    int row_index = 0;
    while (this->prior_op->getNext(&this->tmp_result, row_index))
        row_index++;
    this->total_row_num = row_index;

    quick_sort(result_buffer, 0, row_index - 1);
    current_row = 0;
    return true;
}

void OrderBy::quick_sort(char *buffer, int64_t left, int64_t right)
{
    if (left < right)
    {
        int p = this->partition(buffer, left, right);
        quick_sort(buffer, left, p - 1);
        quick_sort(buffer, p + 1, right);
    }
}
int OrderBy::partition(char *buffer, int64_t left, int64_t right)
{
    char *key = (char *)malloc(this->row_length);
    memcpy(key, get_rowptr(left), this->row_length);
    while (left < right)
    {
        while (left < right && GE(get_rowptr(right), key))
            right--;
        if (left < right)
        {
            memcpy(get_rowptr(left), get_rowptr(right), this->row_length);
            left++;
        }
        while (left < right && !GE(get_rowptr(left), key))
            left++;
        if (left < right)
        {
            memcpy(get_rowptr(right), get_rowptr(left), this->row_length);
            right--;
        }
    }
    memcpy(get_rowptr(left), key, this->row_length);
    free(key);
    return left;
}
bool OrderBy::GE(void *left_ptr, void *right_ptr)
{
    for (int i = 0; i < orderby_num; ++i)
    {
        auto left_ptr_col = left_ptr + compare_col_offset[i];
        auto right_ptr_col = right_ptr + compare_col_offset[i];
        if (this->compare_col_type[i]->cmpEQ(left_ptr_col, right_ptr_col))
            continue;
        if (this->compare_col_type[i]->cmpGT(left_ptr_col, right_ptr_col))
            return true;
        else
            return false;
    }
    return true;
}
bool OrderBy::getNext(ResultTable *result)
{
    return getNext(result, 0);
}
bool OrderBy::getNext(ResultTable *result, int64_t row_num)
{
    if (current_row >= total_row_num)
        return false;
    for (int i = 0; i < this->table_out_col_num; i++)
    {
        char *buffer = this->tmp_result.getRC(current_row, i);
        result->writeRC(row_num, i, buffer);
    }
    current_row++;
    return true;
}
bool OrderBy::close()
{
    tmp_result.shut();
    printf("OrderBy close\n");
    return prior_op->close();
}
