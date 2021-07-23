/*
 * 版权所有 (c) 华为技术有限公司 2019-2019
 * 功能描述   :
 * 1. MDC UART消息实例化demo样例代码（基于ARXML)
 * 2. 需要结合传感器的硬件实际配置和物理连线进修改
 * 3. 此代码需基于不同开发工具和环境配置进行调测
 * 作    者  :
 * 生成日期   : 2019-08-16
 */
#include <stdint.h>
#include <iostream>
#include <string>
#include <fstream>
#include "uart.h"

using namespace ara::log;

UartDataReceive::UartDataReceive()
{
    m_uartProxy = nullptr;
    ProxyUart::StartFindService(
        [this](ara::com::ServiceHandleContainer<ProxyUart::HandleType> handles, ara::com::FindServiceHandle handler) {
            UartDataReceive::ServiceAvailabilityCallback(std::move(handles), handler);
        },
        1);
}

UartDataReceive::~UartDataReceive()
{
}

void UartDataReceive::ServiceAvailabilityCallback(
    ara::com::ServiceHandleContainer<ProxyUart::HandleType> handles, ara::com::FindServiceHandle handler)
{
    m_logger.LogInfo() << "ServiceAvailabilityCallbackUart init size:" << handles.size();
    for (auto it : handles) {
        m_logger.LogInfo() << "Uart Instance " << static_cast<uint16_t>(it.GetInstanceId()) << " is available";
    }
    if (handles.size() > 0) {
        std::lock_guard<std::mutex> lock(m_proxyMutex);
        int instanceId = static_cast<uint16_t>(handles[0].GetInstanceId());

        if (m_uartProxy == nullptr) {
            m_uartProxy = std::make_unique<ProxyUart>(handles[0]);
            m_logger.LogInfo() << "Created uart proxy from handle with instance: " << instanceId;
            m_uartProxy->UartDataRxEvent.SetReceiveHandler([this]() { UartDataReceive::UartDataEventReceived(); });
            m_uartProxy->UartDataRxEvent.Subscribe(ara::com::EventCacheUpdatePolicy::kNewestN, SUBSCRIBE_NUM);
            m_uartMethodThread = std::make_unique<std::thread>(&UartDataReceive::UartSetData, this);
        }
    }
}
struct BestPosHeader {
  uint8_t sync[3];
  uint8_t header_len;
  uint16_t msg_id;
  uint8_t msg_type;
  uint8_t reserved0;
  uint16_t msg_len;
  uint16_t reserved1;
  uint8_t idle_time;
  uint8_t quality;
  uint16_t week;
  uint32_t ms;
  uint32_t reserved2;
  uint16_t diff_sec_gps;
  uint16_t reserved3;
};

struct BestPosMsg {
  uint32_t sol_status;
  uint32_t pos_type;
  double lat;
  double lon;
  double hgt;
  float undulation;
  uint32_t datum;
  float lat_d;
  float lon_d;
  float hgt_d;
  uint8_t stn_id[4];
  float diff_age;
  float sol_age;
  uint8_t tra_count;
  uint8_t sol_count;
  uint8_t reserved[3];
  uint8_t ext_sol_stat;
  uint8_t galilen_mask;
  uint8_t mask;
  uint32_t crc;
};

struct BestPosEvent {
  uint32_t pos_type;
  double lat;
  double lon;
  double hgt;
  float lat_d;
  float lon_d;
  float hgt_d;
};

void UartDataReceive::UartDataEventReceived()
{
    m_uartProxy->UartDataRxEvent.Update();
    const auto& uartSamples = m_uartProxy->UartDataRxEvent.GetCachedSamples();

    for (auto sample : uartSamples) {
    	auto header = reinterpret_cast<BestPosHeader*>(sample->data.data());

    	printf("msg_id %d msg_type %d header_len %d msg_len %d quality %d len %d\n",\
    			header->msg_id,header->msg_type,header->header_len,header->msg_len,header->quality,sample->validLen);

    	auto msg = reinterpret_cast<BestPosMsg*>(sample->data.data()+header->header_len);
    	printf("pos_type %d,lat %ld, lon %ld, hgt %ld, lat_d %ld, lon_d %ld, hgt_d %ld",
    	           msg->pos_type, msg->lat, msg->lon, msg->hgt, msg->lat_d, msg->lon_d, msg->hgt_d);


//    	m_logger.LogInfo() <<"set="<<sample->seq<<" sec="<<sample->timeStamp.sec<<" to len="<<sample->validLen;
//    	m_logger.LogInfo() <<"header len="<<sample->data[4]<<"msg id="<<sample->data[5]+sample->data[6]*8;
//    	m_logger.LogInfo() <<"msg type="<<sample->data[7]<<"msg len="<<sample->data[8];
//    	m_logger.LogInfo() <<"week="<<sample->data[14]+sample->data[15]*8;
//    	m_logger.LogInfo() <<"con="<<static_cast<char*>(sample->data+sample->data[4]);

    }
    m_uartProxy->UartDataRxEvent.Cleanup();
}

void UartDataReceive::UartSetData()
{
    unsigned int i = 0;
    if (m_uartProxy == nullptr) {
        m_logger.LogError() << "m_uartProxy not ready!";
        return;
    }

    UartDataParam param;
    param.seq = UART_MSG_TEST_SEQ;
    param.validLen = UART_DATA_VALID_LEN;

    for (i = 0; i < UART_DATA_MAX_LEN; i++) {
        param.data.push_back(i);
    }

    while (1) {
        param.seq++;
        for (i = 0; i < UART_DATA_MAX_LEN; i++) {
            param.data[i]++;
        }
        auto handle = m_uartProxy->UartDataSetMethod(param);

        if (handle.wait_for(std::chrono::milliseconds(UARTSEND_METHOD_PERIOD)) != ara::core::FutureStatus::kReady) {
            m_logger.LogError() << "set uart data timeout!";
            continue;
        }
        auto output = handle.get();
        m_logger.LogInfo() << "receive uart method result " << static_cast<int>(output.result);

        std::this_thread::sleep_for(std::chrono::milliseconds(UARTSEND_METHOD_PERIOD));
    }
}

