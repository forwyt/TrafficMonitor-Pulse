#include "stdafx.h"
#include "CPUUsage.h"
#include "Common.h"
#include "TrafficMonitor.h"
#include <powerbase.h>
#include <sysinfoapi.h>
#include <algorithm>
#include <cmath>
#include <cwchar>


///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
CPdhCPUUsage::CPdhCPUUsage()
    : CPdhQuery(theApp.m_win_version.GetMajorVersion() >= 10 ? _T("\\Processor Information(_Total)\\% Processor Utility") : _T("\\Processor Information(_Total)\\% Processor Time"))
{
}

bool CPdhCPUUsage::GetCPUUsage(int& cpu_usage)
{
    double value{};
    if (QueryValue(value))
    {
        cpu_usage = static_cast<int>(value);
        if (cpu_usage > 100)
            cpu_usage = 100;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
CCPUUsage::CCPUUsage()
{
}

int CCPUUsage::GetCpuUsage(bool use_cpu_time)
{
    int cpu_usage{};
    if (use_cpu_time)
    {
        cpu_usage = GetCpuUsageByGetSystemTimes();
    }
    else
    {
        //如果通过pdh获取CPU利用率失败，采用GetSystemTimes获取
        if (!m_pdh_cup_usage_query.GetCPUUsage(cpu_usage))
        {
            cpu_usage = GetCpuUsageByGetSystemTimes();
            //写入日志
            //static bool write_log = false;
            //if (!write_log)
            //{
            //    CString str_log = CCommon::LoadTextFormat(IDS_GET_CPU_USAGE_BY_PDH_FAILED_LOG, { fullCounterPath });
            //    CCommon::WriteLog(str_log, theApp.m_log_path.c_str());
            //    write_log = true;
            //}
        }
    }
    return cpu_usage;
}

int CCPUUsage::GetCpuUsageByGetSystemTimes()
{
    int cpu_usage{};
    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    __int64 idle = CCommon::CompareFileTime2(m_preidleTime, idleTime);
    __int64 kernel = CCommon::CompareFileTime2(m_prekernelTime, kernelTime);
    __int64 user = CCommon::CompareFileTime2(m_preuserTime, userTime);

    if (kernel + user == 0)
    {
        cpu_usage = 0;
    }
    else
    {
        //（总的时间-空闲时间）/总的时间=占用cpu的时间就是使用率
        cpu_usage = static_cast<int>(abs((kernel + user - idle) * 100 / (kernel + user)));
    }
    m_preidleTime = idleTime;
    m_prekernelTime = kernelTime;
    m_preuserTime = userTime;

    return cpu_usage;
}

CCPUUsage::LogicalCpuQuery::LogicalCpuQuery()
    : CPdhQuery(L"\\Processor Information(*)\\% Processor Time")
{
}

std::vector<int> CCPUUsage::LogicalCpuQuery::Sample()
{
    std::vector<CounterValueItem> counters;
    if (!QueryValues(counters))
        return {};

    // Processor Information instances are named "group,index", including on machines
    // with more than one processor group. Aggregate instances have no numeric index.
    std::map<std::pair<unsigned, unsigned>, int> ordered_usage;
    for (const auto& counter : counters)
    {
        unsigned group{}, index{};
        if (swscanf_s(counter.name.c_str(), L"%u,%u", &group, &index) != 2)
            continue;

        int usage = -1;
        if ((counter.status == PDH_CSTATUS_VALID_DATA || counter.status == PDH_CSTATUS_NEW_DATA)
            && std::isfinite(counter.value))
        {
            usage = static_cast<int>(std::lround(std::clamp(counter.value, 0.0, 100.0)));
        }
        ordered_usage[{ group, index }] = usage;
    }

    std::vector<int> usage;
    usage.reserve(ordered_usage.size());
    for (const auto& entry : ordered_usage)
        usage.push_back(entry.second);
    return usage;
}

std::vector<int> CCPUUsage::GetLogicalCpuUsage()
{
    return m_logical_cpu_query.Sample();
}
