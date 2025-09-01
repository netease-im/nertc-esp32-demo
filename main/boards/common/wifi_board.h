#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

#include "board.h"

class WifiBoard : public Board {
protected:
    bool wifi_config_mode_ = false;
    void EnterWifiConfigMode();
#ifdef SUPPORT_BLUFI_FOR_NERTC
    void EnterWifiConfigModeWithBlufi();
#endif
    virtual std::string GetBoardJson() override;

public:
    WifiBoard();
    virtual std::string GetBoardType() override;
    virtual void StartNetwork() override;
    virtual Http* CreateHttp() override;
    virtual WebSocket* CreateWebSocket() override;
    virtual Mqtt* CreateMqtt() override;
    virtual Udp* CreateUdp() override;
    virtual Transport* CreateTcp(bool tls) override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveMode(bool enabled) override;
    virtual void ResetWifiConfiguration();
    virtual void ResetWifiConfigurationWithBlufi();
    virtual AudioCodec* GetAudioCodec() override { return nullptr; }
    virtual std::string GetDeviceStatusJson() override;
};

#endif // WIFI_BOARD_H
