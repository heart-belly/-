
/*
* 后续提交可以改变参数的地方：
* 1. 改变第一次截断同一类型任务的时间
* 2. 改变用户后续任务的归属堆问题（通过判断第一堆里剩余的数量）
*/

/*
* 赛题变动：以最小化完成任务的最迟时间，在构造得分函数的时候可以默认所有机器的运行趋于平均。
*/


// 优化
#pragma GCC optimize(1)
#pragma GCC optimize(2)
#pragma GCC optimize(3)
#include <bits/stdc++.h>

using namespace std;

typedef pair<int, int> PII;
typedef pair<int, pair<int, int>> PIPII;
#define x first
#define y second


const int N = 1e5 + 10, M = 40, C = 4294967295;
const int MSgType = 200 + 10, UsrInst = 1e4 + 10;
const int ExeTime = 4000, Deadline = 4294967295;

struct Task {
    int msgType; // 任务类型
    int usrInst; // 属于用户
    int ExeTime; // 执行时间
    int deadline; // 截至时间

}task[N];

// n : 任务数量; m : 机器数; c : 系统最大处理时间
int n, m, c;
// 用户拥有的任务（统计作用）
unordered_set<int> userHasTask[UsrInst];

int maxUserId; 

struct UserTask {
    // 按照顺序保存每个用户的任务
    vector<int> userTaskList;
    // 执行到哪个任务
    int index; 
    // 累计执行时间（便于修正后续任务的真正执行时间）
    int acc;
} userTask[UsrInst];

/*
* 核上相关信息
*/
int coresTime[M];
// 保存运行在核上的用户编号
vector<vector<int>> coresContainUserId;
// 用于输出在核上运行的任务
vector<vector<PII>> coreResult;
// 堆一（优先级最高）
priority_queue<PIPII, vector<PIPII>, greater<PIPII>> heap[M][MSgType];
// 堆二（优先级次高，里面的有超时的任务也有截至时间距离当前机器运行时间有一定跨度（时间参数可以调）的时间）
priority_queue<PIPII, vector<PIPII>, greater<PIPII>> heapTemp[M][MSgType];
// 堆三（优先级最低，一定是超时的任务，那么就考虑亲和度就行）
priority_queue<PIPII, vector<PIPII>, greater<PIPII>> heapTemp2[M][MSgType];
// 用来存储第一个任务的所有执行时间（用于排序选择）

int coresCanDealTask[M][MSgType];

// 统计每个核的执行时间
int coresTerminal[M];

/*
* 统计每个用户的任务数量
*/

// 统计不同任务类型的数量
vector<vector<PII>> taskNumber(MSgType);
// 统计用户的任务数量
unordered_map<int, int> userHasTaskNumber;
// 统计每个每个用户第一个任务：格式（deadline, userId);
vector<vector<PII>> firstTask(MSgType);

/*
* cnt : 统计总的任务数量
* cnt1 : 未超时的任务数量1
* cnt11 : 未超时的任务数量2
* cnt2 : 亲和性数量（大概）
*/
int cnt, cnt1, cnt11, cnt2;

void debug(); // 调试
void input(); // 输入
void init (); // 初始化
void print(); // 输出结果
void solve(); // 解决方案

int main() {

    input();
    init();
    solve();
    debug();
    // print();

}


void input() {
    // 输入数据第一行
    scanf("%d %d %d", &n, &m, &c);
    // 输入任务
    for (int i = 1; i <= n; i ++ ) {
        int taskCls, userId, excTime, deadTime;
        scanf("%d %d %d %d", &taskCls, &userId, &excTime, &deadTime);
        // // 修正实际的运行时间
        // excTime = (int)(1.0 * excTime * (2.0 - 0.8 * 0.8));

        task[i] = {taskCls, userId, excTime, deadTime};
        task[i].deadline = min(task[i].deadline, c);
        // 用户下保存任务
        userHasTask[userId].insert(i);
        userTask[userId].userTaskList.emplace_back(i);
        userTask[userId].acc += excTime;

        taskNumber[taskCls].push_back({userTask[userId].acc, deadTime});
    }

}

void init () {
    coresContainUserId = vector<vector<int>>(m);
    coreResult = vector<vector<PII>>(m);
    // 未超时任务数量以及亲和节点个数
    cnt = cnt1 = cnt11 = cnt2 = 0;
    // 初始化每格核的运行时间
    for (int i = 0; i < m; i ++ ) {
        coresTime[i] = 1;
    }
}

void solve() {

    for (int i = 0; i < UsrInst; i ++ ) {
        if (userTask[i].userTaskList.size() > 0) {
            int taskId1 = userTask[i].userTaskList[0];
            int cls1 = task[taskId1].msgType;
            firstTask[cls1].push_back({task[taskId1].deadline, i});
        }
    }
    // 将每个类型的任务按照顺序依次分类
    // 得到总的任务数量进行平分
    priority_queue<PII, vector<PII>, greater<PII>> taskClassTag;
    for (int i = 0; i < MSgType; i ++ ) {
        if (firstTask[i].size()) {
            // 计算第一类任务的所有运行时间
            int accTime = 0;
            for (auto &[_, userId] : firstTask[i]) {
                for (auto &t : userTask[userId].userTaskList) {
                    accTime += task[t].ExeTime;
                }
            }
            taskClassTag.push({accTime, i});
        }
    }
    int s = taskClassTag.size();
    // 分配策略（参数可调）
    int d = 1.0 * s / (1.0 * m + 1.85),  r = s - m * d;
    int index = 0;

    priority_queue<PII, vector<PII>, greater<PII>> sortHeap;
    for (int i = 0; i < m; i ++ ) {
        sortHeap.push({coresTerminal[i], i});
    }

    int cores = 0;
    for (int i = 0; i < d * m; i ++) {
        auto[_, cls] = taskClassTag.top();
        taskClassTag.pop();
        // 得到堆顶元素
        auto tt = sortHeap.top();
        sortHeap.pop();
        cores = tt.second;
        for (auto [x, y] : firstTask[cls]) {
            coresContainUserId[cores].push_back(y);
            // 将该用户的执行时间加到核上
            for (int taskId : userTask[y].userTaskList) {
                coresTerminal[cores] += task[taskId].ExeTime; 
            }
        }
        sortHeap.push({coresTerminal[cores], cores});


    }

    // debug
    // for (int i = 0; i < m; i ++ ) {
    //     cout << "core:" << i << " terminalTime" << coresTerminal[i] << endl; 
    // }

    // 分配余数分类给核执行时间最短的
    cores = 0;
    for (int i = 0; i < r; i ++ ) {
        auto[_, cls] = taskClassTag.top();
        taskClassTag.pop();
        // 将对应任务的用户分配到对应的核上取
        for (auto [x, y] : firstTask[cls]) {
            // 得到堆顶元素
            auto tt = sortHeap.top();
            sortHeap.pop();
            cores = tt.second;
            coresContainUserId[cores].push_back(y);
            for (int taskId : userTask[y].userTaskList) {
                coresTerminal[cores] += task[taskId].ExeTime; 
            }
            sortHeap.push({coresTerminal[cores], cores});
        }
    }
    // 依次处理每一个核第一次运行的任务类型
    for (int i = 0; i < m; i ++ ) {
        for (int userId : coresContainUserId[i]) {
            // 核上第一次运行的任务
            int taskId = userTask[userId].userTaskList[0];
            int excTime = task[taskId].ExeTime, deadline = task[taskId].deadline;
            int cls = task[taskId].msgType;
            coresCanDealTask[i][cls] ++;
            // 存入到对应的优先队列中
            heap[i][cls].push({deadline, {excTime, userId}});
        }

    }
    // 在核内寻找具有最多连续类型的任务数量
    for (int i = 0; i < m; i ++ ) {
        bool flag = true; // 表示仍然有任务需要运行

        while (flag) {
            int number = 0, cls = -1;
            // 第一次寻找可以调度的不超时的任务
            for (int j = 0; j < MSgType; j ++ ) {
                if (heap[i][j].size() == 0) continue;

                if (heap[i][j].size() > number) {
                    number = heap[i][j].size();
                    cls = j;
                }
                
            }
            if (number != 0) {
                // 将对应的优先队列中所有任务加载到核上运行
                // 第一遍筛选出超时的任务(和截至时间远大于当前时间的节点)
                bool f1 = false;
                int accCur = 0;
                int sz = heap[i][cls].size();
                while (heap[i][cls].size()) {
                    // 取出优先队列中的任务
                    pair<int, pair<int, int>> t = heap[i][cls].top();
                    heap[i][cls].pop();

                    // 执行任务
                    int excTime = t.y.x, userId = t.y.y, dealineTime = t.x;
                    // 更新用户任务列表
                    int &index = userTask[userId].index;
                    int taskId =  userTask[userId].userTaskList[index];


                    // 判断当前时间有没有超过运行任务的截至时间（debug)
                    if (dealineTime > coresTime[i] + task[taskId].ExeTime ) {
                        // 参数可调
                        if (dealineTime > coresTime[i] + 1.0 * 1500  * 200 && accCur >= 1.0 * sz / 2.5)  {
                            heapTemp[i][cls].push({dealineTime, {excTime, userId}});
                            continue;
                        } else {
                            cnt1 ++;
                        }
                    }
                    else {
                        // 超时的任务
                        heapTemp[i][cls].push({INT_MAX, {excTime, userId}});
                        continue;
                    }

                    // 实际运行的任务
                    accCur ++;
                    // 调试总节点数
                    cnt ++;
                    // 亲和性相加
                    cnt2 ++;
                    // 有亲和任务
                    f1 = true;

                    // 更新当前核的运行时间
                    coresTime[i] += task[taskId].ExeTime;

                    index ++;

                    // 更新新的任务序列
                    if (index < userTask[userId].userTaskList.size()) {
                        // 更新任务类型
                        int nextTaskId = userTask[userId].userTaskList[index];
                        int nextCls = task[nextTaskId].msgType;
                        // 更新优先队列
                        int putTime = (coresTime[i] + task[nextTaskId].ExeTime) > task[nextTaskId].deadline ? INT_MAX : task[nextTaskId].deadline;
                        // 参数可调
                        if (heap[i][nextCls].size() >= 2 && putTime != INT_MAX) {
                            // 放入队列一
                            heap[i][nextCls].push({putTime, {task[nextTaskId].ExeTime, userId}});
                        }
                        else {

                            heapTemp[i][nextCls].push({putTime, {task[nextTaskId].ExeTime, userId}});
                        }

                    }
                    // 放在结果中
                    coreResult[i].push_back({task[taskId].msgType, userId});
                                    
                }

                // 有调度未超时的任务
                if (f1) {
                    cnt2 -= 1;
                }
        
            }

            // 在可以调度的任务中找不到可以不超时的任务
            if (number == 0) {
                // 第一次寻找可以调度的不超时的任务
                for (int j = 0; j < MSgType; j ++ ) {
                    if (heapTemp[i][j].size() == 0) continue;

                    if (heapTemp[i][j].size() > number) {
                        number = heapTemp[i][j].size();
                        cls = j;
                    }
                    
                }

                if (heapTemp[i][cls].size()) cnt2 += heapTemp[i][cls].size() - 1;

                while (heapTemp[i][cls].size()) {
                    // 取出优先队列中的任务
                    pair<int, pair<int, int>> t = heapTemp[i][cls].top();
                    heapTemp[i][cls].pop();

                    // 执行任务
                    int excTime = t.y.x, userId = t.y.y, dealineTime = t.x;
                    // 更新用户任务列表
                    int &index = userTask[userId].index;
                    int taskId =  userTask[userId].userTaskList[index];


                    // 判断当前时间有没有超过运行任务的截至时间（debug)
                    if (task[taskId].deadline > coresTime[i] + task[taskId].ExeTime ) {
                        cnt1 ++;
                    } else {
                        heapTemp2[i][cls].push({dealineTime, {excTime, userId}});
                        continue;
                    }

                    // 调试总节点数
                    cnt ++;
   
                    // 更新当前核的运行时间
                    coresTime[i] += task[taskId].ExeTime;

                    index ++;

                    // 更新新的任务序列
                    if (index < userTask[userId].userTaskList.size()) {
                        // 更新任务类型
                        int nextTaskId = userTask[userId].userTaskList[index];
                        int nextCls = task[nextTaskId].msgType;
                        // 更新优先队列
                        int putTime = (coresTime[i] + task[nextTaskId].ExeTime) > task[nextTaskId].deadline ? INT_MAX : task[nextTaskId].deadline;
                        // 参数可调
                        if (heap[i][nextCls].size() >= 1 && putTime != INT_MAX) {
                            // 放入队列一
                            heap[i][nextCls].push({putTime, {task[nextTaskId].ExeTime, userId}});
                        }
                        else {

                            heapTemp[i][nextCls].push({putTime, {task[nextTaskId].ExeTime, userId}});
                        }

                    }

                    // 放在结果中
                    coreResult[i].push_back({task[taskId].msgType, userId});
                                    
                }
                // 处理超时任务
                while (heapTemp2[i][cls].size()) {
                    // 取出优先队列中的任务
                    pair<int, pair<int, int>> t = heapTemp2[i][cls].top();
                    heapTemp2[i][cls].pop();
                    // 执行任务
                    int excTime = t.y.x, userId = t.y.y;
                    // 更新用户任务列表
                    int &index = userTask[userId].index;
                    int taskId =  userTask[userId].userTaskList[index];

                    
                    // 判断当前时间有没有超过运行任务的截至时间（debug)
                    if (task[taskId].deadline > coresTime[i] + task[taskId].ExeTime ) cnt11 ++;
                    // 调试总节点数
                    cnt ++;
           
                    // 更新当前核的运行时间
                    coresTime[i] += task[taskId].ExeTime;

                    index ++;

                    // 更新新的任务序列
                    if (index < userTask[userId].userTaskList.size()) {
                        // 更新任务类型
                        int nextTaskId = userTask[userId].userTaskList[index];
                        int nextCls = task[nextTaskId].msgType;
                        // 更新优先队列
                        int putTime = (coresTime[i] + task[nextTaskId].ExeTime) > task[nextTaskId].deadline ? INT_MAX : (task[nextTaskId].deadline - coresTime[i]) / task[nextTaskId].ExeTime;
                        heapTemp[i][nextCls].push({putTime, {task[nextTaskId].ExeTime, userId}});

                    }

                    // 放在结果中
                    coreResult[i].push_back({task[taskId].msgType, userId});

                }                
            }

            if (number == 0) {
                flag = false;
            }
        }
    }
    
}

void print() {
    // 4.输出结果，使用字符串存储，一次IO输出
    stringstream out;
    for (int coreId = 0; coreId < m; ++coreId) {
        out << coreResult[coreId].size();
        for (auto &task : coreResult[coreId]) {
            out << " " << task.x << " " << task.y;
        }
        out << endl;
    }
    printf("%s", out.str().c_str());

}


void debug() {
    /**************************************************************************************/
    // 输出未超时的节点数量以及亲和调度的节点数量
    cout << "can not timeout node : " << cnt1 << "can not timeout node : " << cnt11 << endl;
    cout << "affinity node : " << cnt2 << endl;
    cout << "affinity node + can not timeout node :" << cnt1 + cnt2 + cnt11 << endl;
    cout << "total node number :" << cnt << endl;
    /**************************************************************************************/

    int terminalTime = 0;
    for (int i = 0; i < m; i ++ ) {
        cout << "time: " << coresTime[i] << endl;
        terminalTime = max(terminalTime, coresTime[i]);
    }
    cout << "terminal time : " << terminalTime << endl;
    int cnt = 0;
    // 依次输出每个用户的任务序列
    for (int i = 0; i < UsrInst ; i ++ ) {
        // 没有任务
        if ( userTask[i].userTaskList.empty()) {
            continue;
        }
        cnt ++;
        printf("userId: %d\n", i);
        for (auto &taskId : userTask[i].userTaskList) {
            printf("taskClass : %d, excTime : %d, deadTime : %d,\n", task[taskId].msgType, task[taskId].ExeTime, task[taskId].deadline);
        }
        printf("\n");

    }
    /**************************************************************************************/
    for (int i = 0; i < UsrInst; i ++ ) {
        if ( userTask[i].userTaskList.empty()) continue;
        userHasTaskNumber[userTask[i].userTaskList.size()] ++;
    }
    for (auto [x, y] : userHasTaskNumber) {
        cout << "the number of task :" << x << " the number of user" << y << endl;
    }

    /**************************************************************************************/
    // 输入不同任务类型数量
    for (int i = 0; i < MSgType; i ++ ) {
        if (taskNumber[i].size() == 0) continue;
        cout << "taskClass :" << i << "number :" << taskNumber[i].size() << endl;
        for (auto[x, y] : taskNumber[i]) {
            cout << "taskEXetime :" << x << " taskDeadline" << y << endl;
            cout << endl;
        }
        cout << endl;
    }

    /**************************************************************************************/
    vector<vector<PII>> temp(MSgType);

    // 输出每个用户的任务数量
    for (int i = 0; i <  UsrInst; i ++ ) {
        if (userTask[i].userTaskList.size() == 1) {
            int taskId = userTask[i].userTaskList[0];
            int cls = task[taskId].msgType;
            temp[cls].push_back({task[taskId].deadline, task[taskId].ExeTime});

        }

    }
    // 输出单个任务的执行时间
    for (int i = 0; i < MSgType; i ++ ) {
        if (temp[i].size() == 0) continue;
        sort(temp[i].begin(), temp[i].end());
        cout << "taskClass :" << i << "number :" << temp[i].size() << endl;
        for (auto[x, y] : temp[i]) {
            cout << "taskEXetime :" << y << " taskDeadline" << x << endl;
            cout << endl;
        }
        cout << endl;
    }
    /**************************************************************************************/
    // 统计任务数量为2的任务类型
    vector<vector<PII>> temp1(MSgType);
    for (int i = 0; i < UsrInst; i ++ ) {
        if (userTask[i].userTaskList.size() > 0) {
            int taskId1 = userTask[i].userTaskList[0];
            int cls1 = task[taskId1].msgType;
            temp1[cls1].push_back({task[taskId1].deadline, i});
        }
    }

    for (int i = 0; i < MSgType; i ++ ) {
        if (temp1[i].size() == 0) continue;
        sort(temp1[i].begin(), temp1[i].end());
        cout << "class : " << i << " number of class first : " << temp1[i].size() << endl;
        for (auto [x, y] : temp1[i]) {
            cout << "userId : " << y << " deadlinetime : " << x << endl;
        }
    }

    cout << endl;

    /**************************************************************************************/
    // 输出每个用户的任务数量
    vector<vector<PII>> temp(MSgType);
    for (int i = 0; i <  UsrInst; i ++ ) {
        if (userTask[i].userTaskList.size() > 0) {
            for (int id : userTask[i].userTaskList) {
                int taskId = id;
                int cls = task[taskId].msgType;
                temp[cls].push_back({task[taskId].deadline, task[taskId].ExeTime});
            }

        }
    }
    // unordered_set<int> uset = {6, 78, 98, 87, 69, 5, 84, 81, 15, 124};
    // // 输出单个任务的执行时间
    // for (int i = 0; i < MSgType; i ++ ) {
    //     if (temp[i].size() == 0) continue;
    //     sort(temp[i].begin(), temp[i].end());
    //     cout << "taskClass :" << i << "number :" << temp[i].size() << endl;
    //     for (auto[x, y] : temp[i]) {
    //         cout << "taskEXetime :" << y << " taskDeadline" << x << endl;
    //         cout << endl;
    //     }
    //     cout << endl;
    // }
    /**************************************************************************************/
    // vector<PII> temp1(MSgType);
    // for (int i = 0; i < MSgType; i ++ ) {
    //     if (temp[i].size() == 0) {
    //         temp1[i] = {INT_MAX, INT_MAX};
    //         continue;
    //     }
    //     sort(temp[i].begin(), temp[i].end());
    //     temp1[i] = {temp[i][0].x, temp[i][1].x};
    // }
    // sort(temp1.begin(), temp1.end());
    // for (int i = 0; i < MSgType; i ++ ) {
    //     cout << " first deadline time : " << temp1[i].x << "second deadline time:" << temp1[i].y << endl;

    // }
    

}
