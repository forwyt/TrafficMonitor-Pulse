#pragma once
#include <Pdh.h>
#include <PdhMsg.h>
#include "PdhQuery.h"

class CPdhCPUUsage : public CPdhQuery
{
public:
    CPdhCPUUsage();

    ~CPdhCPUUsage()
    {
    }

    bool GetCPUUsage(int& cpu_usage);

};

//////////////////////////////////////////////////////////////////////////////////
class CCPUUsage
{
public:
    CCPUUsage();

    ~CCPUUsage()
    {}

    int GetCpuUsage(bool use_cpu_time);
    std::vector<int> GetLogicalCpuUsage();

private:
    int GetCpuUsageByGetSystemTimes();

private:

    FILETIME m_preidleTime{};
    FILETIME m_prekernelTime{};
    FILETIME m_preuserTime{};

    CPdhCPUUsage m_pdh_cup_usage_query;

    class LogicalCpuQuery : public CPdhQuery
    {
    public:
        LogicalCpuQuery();
        std::vector<int> Sample();
    };

    LogicalCpuQuery m_logical_cpu_query;
};
