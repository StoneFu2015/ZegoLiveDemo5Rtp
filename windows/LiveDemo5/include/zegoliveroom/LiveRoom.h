//
//  ZegoLiveRoom.h
//  zegoliveroom
//
//  Copyright © 2017年 Zego. All rights reserved.
//

#ifndef ZegoLiveRoom_h
#define ZegoLiveRoom_h

#include "./LiveRoomDefines.h"
#include "./LiveRoomCallback.h"
#include "./LiveRoomDefines-IM.h"

namespace ZEGO
{
	namespace LIVEROOM
	{
        ZEGO_API const char* GetSDKVersion();
        ZEGO_API const char* GetSDKVersion2();
        ZEGO_API bool SetNetType(int nNetType);
        ZEGO_API bool SetLogDir(const char* pszLogDir);
        ZEGO_API void UploadLog();
        ZEGO_API void SetVerbose(bool bVerbose);
        ZEGO_API void SetUseTestEnv(bool bTestEnv);
        ZEGO_API void SetUseAlphaEnv(bool bAlpha);
        ZEGO_API void SetPlatformInfo(const char* pszInfo);

        /**
         设置业务类型

         @param nType 业务类型，取值 0（直播类型）或 2（实时音视频类型）。默认为 0
         @return true 成功，false 失败
         @attention 确保在创建接口对象前调用
         */
        ZEGO_API bool SetBusinessType(int nType);
        
        /**
         是否使用聊天室功能

         @param bChatRoom 是否使用聊天室功能，true 使用，false 不使用。默认为 false
         @return true 成功，false 失败
         @attention zegoliveroom 自带 IM 功能，随 SDK 初始化。如果要额外使用聊天室，需要启用聊天室功能
         */
        ZEGO_API bool SetUseChatRoom(bool bChatRoom);
        
        /**
         设置用户信息

         @param pszUserID 用户唯一 ID
         @param pszUserName 用户名字
         @return true 成功，false 失败
         */
        ZEGO_API bool SetUser(const char* pszUserID, const char* pszUserName);

        /**
         获取用户信息

         @return UserID
         */
        ZEGO_API const char* GetUserID();
        
		/**
		 初始化引擎

		 @param jvm jvm 仅用于 Android
		 @param ctx ctx 仅用于 Android
		 @return true 成功，false 失败
		 */
		ZEGO_API bool InitPlatform(void* jvm = 0, void* ctx = 0);
        
        /**
         初始化 SDK

         @param uiAppID Zego 派发的数字 ID, 各个开发者的唯一标识
         @param pBufAppSignature Zego 派发的签名, 用来校验对应 appID 的合法性
         @param nSignatureSize 签名长度（字节）
         @return true 成功，false 失败
         @note 初始化 SDK 失败可能导致 SDK 功能异常
         */
        ZEGO_API bool InitSDK(unsigned int uiAppID, unsigned char* pBufAppSignature, int nSignatureSize);
        
        /**
         反初始化 SDK

         @return true 成功，false 失败
         */
        ZEGO_API bool UnInitSDK();
        
        /**
         设置直播房间相关信息通知的回调

         @param pCB 回调对象指针
         @return true 成功，false 失败
         */
        ZEGO_API bool SetRoomCallback(IRoomCallback* pCB);

        /**
         设置房间配置信息

         @param audienceCreateRoom 观众是否可以创建房间。true 可以，false 不可以。默认 true
         @param userStateUpdate 用户状态（用户进入、退出房间）是否广播。true 广播，false 不广播。默认 false
         */
        ZEGO_API void SetRoomConfig(bool audienceCreateRoom, bool userStateUpdate);
        
        /**
         登录房间

         @param pszRoomID 房间 ID
         @param role 成员角色
         @param pszRoomName 房间名称
         @return true 成功，false 失败
         */
        ZEGO_API bool LoginRoom(const char* pszRoomID, int role, const char* pszRoomName = "");

        /**
         退出房间

         @return 成功，false 失败
         @attention 退出登录后，等待 IRoomCallback::OnLogoutRoom 回调
         @note 退出房间会停止所有的推拉流
         */
        ZEGO_API bool LogoutRoom();
        
        /**
         发送自定义信令

         @param memberList 信令发送成员列表
         @param memberCount 成员个数
         @param content 信令内容
         @return 消息 seq
         */
        ZEGO_API int SendCustomCommand(ROOM::ZegoUser *memberList, unsigned int memberCount, const char *content);
        
        /**
         设置直播事件回调

         @param pCB 回调对象指针
         */
        ZEGO_API void SetLiveEventCallback(AV::IZegoLiveEventCallback* pCB);
        
        /**
         设置音频视频设备变化的回调

         @param pCB 回调对象指针
         */
        ZEGO_API void SetDeviceStateCallback(AV::IZegoDeviceStateCallback *pCB);
        
        
        //
        // * device
        //

        /**
         获取音频设备列表

         @param deviceType 设备类型
         @param device_count 设备数量
         @return 音频设备列表
         */
        ZEGO_API AV::DeviceInfo* GetAudioDeviceList(AV::AudioDeviceType deviceType, int& device_count);
        
        /**
         设置选用音频设备

         @param deviceType 设备类型
         @param pszDeviceID 设备 ID
         @return true 成功，false 失败
         */
        ZEGO_API bool SetAudioDevice(AV::AudioDeviceType deviceType, const char* pszDeviceID);
        
        /**
         获取视频设备列表

         @param device_count 设备数量
         @return 视频设备列表
         */
        ZEGO_API AV::DeviceInfo* GetVideoDeviceList(int& device_count);
        
        /**
         释放设备列表

         @param parrDeviceList 设备列表
         */
        ZEGO_API void FreeDeviceList(AV::DeviceInfo* parrDeviceList);
        
        /**
         设置选用视频设备

         @param pszDeviceID 设备 ID
         @return true 成功，false 失败
         */
        ZEGO_API bool SetVideoDevice(const char* pszDeviceID, AV::PublishChannelIndex idx = AV::PUBLISH_CHN_MAIN);
        
#ifdef WIN32
        /**
         系统声卡声音采集开关

         @param bEnable true 打开，false 失败
         */
        ZEGO_API void EnableMixSystemPlayout(bool bEnable);
        
        /**
         获取麦克风音量

         @param deviceId 麦克风 deviceId
         @return -1: 获取失败，0~100 麦克风音量
         @note 切换麦克风后需要重新获取麦克风音量
         */
        ZEGO_API int GetMicDeviceVolume(const char *deviceId);
        
        /**
         设置麦克风音量

         @param deviceId 麦克风 deviceId
         @param volume 音量，取值(0,100)
         */
        ZEGO_API void SetMicDeviceVolume(const char *deviceId, int volume);
        
        /**
         获取麦克风是否静音
         
         @param deviceId 麦克风 deviceId
         @return true 静音，false 不静音
         */
        ZEGO_API bool GetMicDeviceMute(const char *deviceId);
        
        /**
         设置麦克风静音
         
         @param deviceId 麦克风 deviceId
         @param mute true 静音，false 不静音
         */
        ZEGO_API void SetMicDeviceMute(const char *deviceId, bool mute);
        
        /**
         获取扬声器音量

         @param deviceId 扬声器 deviceId
         @return -1: 获取失败，0~100 扬声器音量
         @note 切换扬声器后需要重新获取音量
         */
        ZEGO_API int GetSpeakerDeviceVolume(const char *deviceId);
        
        /**
         设置扬声器音量

         @param deviceId 扬声器 deviceId
         @param volume 音量，取值 (0，100)
         */
        ZEGO_API void SetSpeakerDeviceVolume(const char *deviceId, int volume);
        
        /**
         获取 App 中扬声器音量

         @param deviceId 扬声器 deviceId
         @return -1: 获取失败，0~100 扬声器音量
         */
        ZEGO_API int GetSpeakerSimpleVolume(const char *deviceId);
        
        /**
         设置 App 中扬声器音量

         @param deviceId 扬声器 deviceId
         @param volume 音量，取值 (0，100)
         */
        ZEGO_API void SetSpeakerSimpleVolume(const char *deviceId, int volume);

        /**
         获取扬声器是否静音

         @param deviceId 扬声器 deviceId
         @return true 静音，false 不静音
         */
        ZEGO_API bool GetSpeakerDeviceMute(const char *deviceId);
        
        /**
         设置扬声器静音

         @param deviceId 扬声器 deviceId
         @param mute true 静音，false 不静音
         */
        ZEGO_API void SetSpeakerDeviceMute(const char *deviceId, bool mute);
        
        /**
         获取 App 中扬声器是否静音

         @param deviceId 扬声器 deviceId
         @return true 静音，false 不静音
         */
        ZEGO_API bool GetSpeakerSimpleMute(const char *deviceId);
        
        /**
         设置 App 中扬声器静音

         @param deviceId 扬声器 deviceId
         @param mute true 静音，false 不静音
         */
        ZEGO_API void SetSpeakerSimpleMute(const char *deviceId, bool mute);
        
        /**
         获取默认的音频设备

         @param deviceType 音频类型
         @param deviceId 设备 Id
         @param deviceIdLength deviceId 字符串分配的长度
         @note 如果传入的字符串 buffer 长度小于默认 deviceId 的长度，则 deviceIdLength 返回实际需要的字符串长度
         */
        ZEGO_API void GetDefaultAudioDeviceId(AV::AudioDeviceType deviceType, char *deviceId, unsigned int *deviceIdLength);

        /**
         监听设备的音量变化

         @param deviceType 音频类型
         @param deviceId 设备 Id
         @return true 成功，false 失败
         @note 设置后如果有音量变化（包括 app 音量）通过 IZegoDeviceStateCallback::OnAudioVolumeChanged 回调
         */
        ZEGO_API bool SetAudioVolumeNotify(AV::AudioDeviceType deviceType, const char *deviceId);
        
        /**
         停止监听设备的音量变化

         @param deviceType 设备类型
         @param deviceId 设备 Id
         @return true 成功，false 失败
         */
        ZEGO_API bool StopAudioVolumeNotify(AV::AudioDeviceType deviceType, const char *deviceId);

#endif // WIN32
        /**
         设置“音视频引擎状态通知”的回调

         @param pCB 回调对象指针
         @return true 成功，false 失败
         */
        ZEGO_API bool SetAVEngineCallback(IAVEngineCallback* pCB);
        
        /**
         设置配置信息

         @param config config 配置信息
         @attention 确保在 InitSDK 前调用，但开启拉流加速(config为“prefer_play_ultra_source=1”)可在 InitSDK 之后，拉流之前调用
         */
        ZEGO_API void SetConfig(const char *config);
	}
}

#endif
