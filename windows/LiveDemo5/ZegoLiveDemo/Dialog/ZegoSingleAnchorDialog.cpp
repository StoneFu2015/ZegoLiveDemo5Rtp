﻿#include "ZegoSingleAnchorDialog.h"
#include "Signal/ZegoSDKSignal.h"
#include <QMessageBox>
#include <QDebug>
//Objective-C
#ifdef Q_OS_MAC
#include "OSX_Objective-C/ZegoAVDevice.h"
#include "OSX_Objective-C/ZegoCGImageToQImage.h"
#endif
ZegoSingleAnchorDialog::ZegoSingleAnchorDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	//UI的信号槽
	connect(ui.m_bMin, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);
	connect(ui.m_bMax, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);
	connect(ui.m_bClose, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);

}

ZegoSingleAnchorDialog::ZegoSingleAnchorDialog(qreal dpi, SettingsPtr curSettings, RoomPtr room, QString curUserID, QString curUserName, QDialog *lastDialog, QDialog *parent)
	: m_dpi(dpi),
	m_pAVSettings(curSettings),
	m_pChatRoom(room),
	m_strCurUserID(curUserID),
	m_strCurUserName(curUserName),
	m_bCKEnableMic(true),
	m_bCKEnableSpeaker(true),
	m_lastDialog(lastDialog)
{
	ui.setupUi(this);


	//通过sdk的信号连接到本类的槽函数中
	connect(GetAVSignal(), &QZegoAVSignal::sigLoginRoom, this, &ZegoSingleAnchorDialog::OnLoginRoom);
	connect(GetAVSignal(), &QZegoAVSignal::sigPublishStateUpdate, this, &ZegoSingleAnchorDialog::OnPublishStateUpdate);
	connect(GetAVSignal(), &QZegoAVSignal::sigDisconnect, this, &ZegoSingleAnchorDialog::OnDisconnect);
	connect(GetAVSignal(), &QZegoAVSignal::sigKickOut, this, &ZegoSingleAnchorDialog::OnKickOut);
	connect(GetAVSignal(), &QZegoAVSignal::sigPublishQualityUpdate, this, &ZegoSingleAnchorDialog::OnPublishQualityUpdate);

	//信号与槽同步执行
	connect(GetAVSignal(), &QZegoAVSignal::sigAuxInput, this, &ZegoSingleAnchorDialog::OnAVAuxInput, Qt::DirectConnection);
#if (defined Q_OS_WIN) && (defined USE_SURFACE_MERGE)
	connect(GetAVSignal(), &QZegoAVSignal::sigSurfaceMergeResult, this, &ZegoSingleAnchorDialog::OnSurfaceMergeResult, Qt::DirectConnection);
#endif
	connect(GetAVSignal(), &QZegoAVSignal::sigPreviewSnapshot, this, &ZegoSingleAnchorDialog::OnPreviewSnapshot, Qt::DirectConnection);

	connect(GetAVSignal(), &QZegoAVSignal::sigSendRoomMessage, this, &ZegoSingleAnchorDialog::OnSendRoomMessage);
	connect(GetAVSignal(), &QZegoAVSignal::sigRecvRoomMessage, this, &ZegoSingleAnchorDialog::OnRecvRoomMessage);
	connect(GetAVSignal(), &QZegoAVSignal::sigUserUpdate, this, &ZegoSingleAnchorDialog::OnUserUpdate);
	connect(GetAVSignal(), &QZegoAVSignal::sigAudioDeviceChanged, this, &ZegoSingleAnchorDialog::OnAudioDeviceChanged);
	connect(GetAVSignal(), &QZegoAVSignal::sigVideoDeviceChanged, this, &ZegoSingleAnchorDialog::OnVideoDeviceChanged);

	//UI的信号槽
	connect(ui.m_bMin, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);
	connect(ui.m_bMax, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);
	connect(ui.m_bClose, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnClickTitleButton);

	connect(ui.m_bSendMessage, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonSendMessage);

	connect(ui.m_bCapture, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonSoundCapture);

	connect(ui.m_bProgMircoPhone, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonMircoPhone);
	connect(ui.m_bSound, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonSound);
	connect(ui.m_bShare, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnShareLink);
	connect(ui.m_bAux, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonAux);
	connect(ui.m_bRequestJoinLive, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonSwitchPublish);
	connect(ui.m_bFullScreen, &QPushButton::clicked, this, &ZegoSingleAnchorDialog::OnButtonShowFullScreen);
	connect(ui.m_cbMircoPhone, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSwitchAudioDevice(int)));
	connect(ui.m_cbCamera, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSwitchVideoDevice(int)));
	connect(ui.m_cbCamera2, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSwitchVideoDevice2(int)));

#ifdef Q_OS_WIN
	connect(&hookDialog, &ZegoMusicHookDialog::sigUseDefaultAux, this, &ZegoSingleAnchorDialog::OnUseDefaultAux);
	connect(&hookDialog, &ZegoMusicHookDialog::sigSendMusicAppPath, this, &ZegoSingleAnchorDialog::OnGetMusicAppPath);
#endif
		
	connect(this, &ZegoSingleAnchorDialog::sigShowSnapShotImage, this, &ZegoSingleAnchorDialog::OnShowSnapShotImage);

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &ZegoSingleAnchorDialog::OnProgChange);

	ui.m_edInput->installEventFilter(this);
	ui.m_zoneLiveView_Inner->installEventFilter(this);

	//混音数据参数
	m_pAuxData = NULL;
	m_nAuxDataLen = 0;
	m_nAuxDataPos = 0;

	this->setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏 

	gridLayout = new QGridLayout();
	gridLayout->setSpacing(0);
	gridLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
	ui.zoneLiveViewHorizontalLayout->addLayout(gridLayout);

	//单主播模式单画面设置
	m_mainLiveView = new QZegoAVView(ZEGODIALOG_SingleAnchor);
	m_mainLiveView->setCurUser();
	connect(m_mainLiveView, &QZegoAVView::sigSnapShotPreviewOnSingleAnchor, this, &ZegoSingleAnchorDialog::OnSnapshotPreview);
	m_mainLiveView->setMinimumSize(QSize(960, 540));
	m_mainLiveView->setStyleSheet(QLatin1String("border: none;\n"
		"background-color: #383838;"));

	//单主播模式下，推第二路流
	m_AuxLiveView = new QZegoAVView(ZEGODIALOG_SingleAnchor);
	m_AuxLiveView->setCurUser();
	connect(m_AuxLiveView, &QZegoAVView::sigSnapShotPreviewOnSingleAnchor, this, &ZegoSingleAnchorDialog::OnSnapshotPreview);
	m_AuxLiveView->setMinimumSize(QSize(480, 540));
	m_AuxLiveView->setStyleSheet(QLatin1String("border: none;\n"
		"background-color: #383838;"));

	//QMdiArea *view = new QMdiArea;
	//m_mainLiveView->setWindowFlags(Qt::FramelessWindowHint);
	//QLabel *label = new QLabel;
	//label->setText(tr("日你妈耶"));
	//view->addSubWindow(m_mainLiveView);
	//view->addSubWindow(label);
	
	gridLayout->addWidget(m_mainLiveView, 0, 0, 1, 1);
}

ZegoSingleAnchorDialog::~ZegoSingleAnchorDialog()
{

}

//功能函数
void ZegoSingleAnchorDialog::initDialog()
{
	//在mac系统下不支持声卡采集
#ifdef Q_OS_MAC
	ui.m_bCapture->setVisible(false);
#endif
	//在主播端，请求连麦的按钮变为直播开关
	ui.m_bRequestJoinLive->setText(QStringLiteral("停止直播"));

	initComboBox();

	//对话框模型初始化
	m_chatModel = new QStandardItemModel(this);
	ui.m_listChat->setModel(m_chatModel);
	ui.m_listChat->horizontalHeader()->setVisible(false);
	ui.m_listChat->verticalHeader()->setVisible(false);
	ui.m_listChat->verticalHeader()->setDefaultSectionSize(26);
	ui.m_listChat->setItemDelegate(new NoFocusFrameDelegate(this));
	ui.m_listChat->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.m_listChat->setColumnWidth(0, 300);
	

	//成员列表初始化
	m_memberModel = new QStringListModel(this);
	ui.m_listMember->setModel(m_memberModel);
	ui.m_listMember->setItemDelegate(new NoFocusFrameDelegate(this));
	ui.m_listMember->setEditTriggers(QAbstractItemView::NoEditTriggers);

	//读取标题内容
	QString strTitle = QString(QStringLiteral("【%1】%2")).arg(QStringLiteral("单主播模式")).arg(m_pChatRoom->getRoomName());
	ui.m_lbRoomName->setText(strTitle);

	//剩余能用的AVView
	for (int i = MAX_VIEW_COUNT; i >= 0; i--)
		m_avaliableView.push_front(i);

	AVViews.push_back(m_mainLiveView);
	AVViews.push_back(m_AuxLiveView);

	//推流成功前不能开混音、声音采集、分享、停止直播、切换设备
	ui.m_bAux->setEnabled(false);
	ui.m_bCapture->setEnabled(false);
	ui.m_bShare->setEnabled(false);
	ui.m_bRequestJoinLive->setEnabled(false);
	ui.m_cbMircoPhone->setEnabled(false);
	ui.m_cbCamera->setEnabled(false);

	ui.m_cbCamera2->setVisible(false);
	ui.m_lbCamera2->setVisible(false);

	//允许使用麦克风
	LIVEROOM::EnableMic(m_bCKEnableMic);

	//枚举音视频设备
	EnumVideoAndAudioDevice();

	int role = LIVEROOM::ZegoRoomRole::Anchor;
	if (!LIVEROOM::LoginRoom(m_pChatRoom->getRoomId().toStdString().c_str(), role, m_pChatRoom->getRoomName().toStdString().c_str()))
	{
		QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("进入房间失败"));
	}

}

void ZegoSingleAnchorDialog::StartPublishStream()
{

	QTime currentTime = QTime::currentTime();
	//获取当前时间的毫秒
	int ms = currentTime.msec();
	QString strStreamId;
#ifdef Q_OS_WIN
	strStreamId = QString(QStringLiteral("s-windows-%1-%2")).arg(m_strCurUserID).arg(ms);
#else
	strStreamId = QString(QStringLiteral("s-mac-%1-%2")).arg(m_strCurUserID).arg(ms);
#endif
	m_strPublishStreamID = strStreamId;
	
	StreamPtr pPublishStream(new QZegoStreamModel(m_strPublishStreamID, m_strCurUserID, m_strCurUserName, "", true));
    
	m_pChatRoom->addStream(pPublishStream);

	bool isUsePublish2Stream = m_pAVSettings->GetUsePublish2Stream();

	QString strStreamId2 = "";
	StreamPtr pPublishStream2(new QZegoStreamModel(strStreamId2, m_strCurUserID, m_strCurUserName, "", true));

	if (isUsePublish2Stream)
	{
		QTime currentTime = QTime::currentTime();
		//获取当前时间的毫秒
		int ms = currentTime.msec();
		
#ifdef Q_OS_WIN
		strStreamId2 = QString(QStringLiteral("s-windows-%1-%2-aux")).arg(m_strCurUserID).arg(ms);
#else
		strStreamId2 = QString(QStringLiteral("s-mac-%1-%2-aux")).arg(m_strCurUserID).arg(ms);
#endif
		pPublishStream2->setStreamID(strStreamId2);

		m_strPublishStreamID_Aux = strStreamId2;

		m_pChatRoom->addStream(pPublishStream2);
	}
	
	/*qDebug() << "room stream count = " << m_pChatRoom->getStreamCount();
	for (int i = 0; i < m_pChatRoom->getStreamList().size(); ++i)
	{
		qDebug() << "stream = " << m_pChatRoom->getStreamList()[i]->getStreamId();
	}*/

	//推流前调用双声道
	LIVEROOM::SetAudioChannelCount(2);


	if (m_avaliableView.size() > 0)
	{
		
		int nIndex = takeLeastAvaliableViewIndex();
		pPublishStream->setPlayView(nIndex);
		qDebug() << "publish nIndex = " << nIndex;
		int nIndex2 = -1;
		if (isUsePublish2Stream)
		{
			if (m_avaliableView.size() <= 0)
				return;

			nIndex2 = takeLeastAvaliableViewIndex();
			pPublishStream2->setPlayView(nIndex2);

			qDebug() << "publish2 nIndex = " << nIndex2;
		}

		if (m_pAVSettings->GetSurfaceMerge())
		{
#if (defined Q_OS_WIN) && (defined USE_SURFACE_MERGE) 
			StartSurfaceMerge();
#endif
		}
		else
		{
			LIVEROOM::SetVideoFPS(m_pAVSettings->GetFps());
			LIVEROOM::SetVideoBitrate(m_pAVSettings->GetBitrate());
			LIVEROOM::SetVideoCaptureResolution(m_pAVSettings->GetResolution().cx, m_pAVSettings->GetResolution().cy);
			LIVEROOM::SetVideoEncodeResolution(m_pAVSettings->GetResolution().cx, m_pAVSettings->GetResolution().cy);

			//配置View
			LIVEROOM::SetPreviewView((void *)AVViews[nIndex]->winId());
			LIVEROOM::SetPreviewViewMode(LIVEROOM::ZegoVideoViewModeScaleAspectFit);
			LIVEROOM::StartPreview();

			if (isUsePublish2Stream)
			{
				m_mainLiveView->setMinimumSize(QSize(480, 540));
				gridLayout->addWidget(m_AuxLiveView, 0, 1, 1, 1);

				
				//LIVEROOM::SetVideoDevice("", ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::SetVideoFPS(m_pAVSettings->GetFps(), ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::SetVideoBitrate(m_pAVSettings->GetBitrate(), ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::SetVideoCaptureResolution(m_pAVSettings->GetResolution().cx, m_pAVSettings->GetResolution().cy, ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::SetVideoEncodeResolution(m_pAVSettings->GetResolution().cx, m_pAVSettings->GetResolution().cy, ZEGO::AV::PUBLISH_CHN_AUX);

				//配置View
				LIVEROOM::SetPreviewView((void *)AVViews[nIndex2]->winId(), ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::SetPreviewViewMode(LIVEROOM::ZegoVideoViewModeScaleAspectFit, ZEGO::AV::PUBLISH_CHN_AUX);
				LIVEROOM::StartPreview(ZEGO::AV::PUBLISH_CHN_AUX);
			}
		}

		if (!isUsePublish2Stream)
		{
			//推流前设置水印(必须在设置好编码分辨率之后)
			setWaterPrint();
			QString streamID = m_strPublishStreamID;
			m_mainLiveView->setViewStreamID(streamID);
			qDebug() << "start publishing!";
			LIVEROOM::StartPublishing(m_pChatRoom->getRoomName().toStdString().c_str(), streamID.toStdString().c_str(), LIVEROOM::ZEGO_SINGLE_ANCHOR, "");
		}
		else
		{
			//推流前设置水印(必须在设置好编码分辨率之后)
			setWaterPrint();
			QString streamID = m_strPublishStreamID;
			m_mainLiveView->setViewStreamID(streamID);
			qDebug() << "start publishing!";
			LIVEROOM::StartPublishing2(m_pChatRoom->getRoomName().toStdString().c_str(), streamID.toStdString().c_str(), LIVEROOM::ZEGO_SINGLE_ANCHOR, "");

			//推流前设置水印(必须在设置好编码分辨率之后)
			setWaterPrint(ZEGO::AV::PUBLISH_CHN_AUX);
			QString streamID2 = m_strPublishStreamID_Aux;
			m_AuxLiveView->setViewStreamID(streamID);
			qDebug() << "start publishing2!";
			LIVEROOM::StartPublishing2(m_pChatRoom->getRoomName().toStdString().c_str(), streamID2.toStdString().c_str(), LIVEROOM::ZEGO_SINGLE_ANCHOR, "", ZEGO::AV::PUBLISH_CHN_AUX);
		}
		m_bIsPublishing = true;
	}
}

void ZegoSingleAnchorDialog::StopPublishStream(const QString& streamID, AV::PublishChannelIndex idx)
{
	if (streamID.size() == 0){ return; }

	if (m_pAVSettings->GetSurfaceMerge())
	{
#if (defined Q_OS_WIN) && (defined USE_SURFACE_MERGE) 
		SurfaceMerge::SetRenderView(nullptr);
		SurfaceMerge::UpdateSurface(nullptr, 0);
#endif
	}
	else
	{
		if (idx == ZEGO::AV::PUBLISH_CHN_MAIN)
		{
			LIVEROOM::SetPreviewView(nullptr);
			LIVEROOM::StopPreview();
		}
		else
		{
			LIVEROOM::SetPreviewView(nullptr, ZEGO::AV::PUBLISH_CHN_AUX);
			LIVEROOM::StopPreview(ZEGO::AV::PUBLISH_CHN_AUX);
		}
	}

	LIVEROOM::StopPublishing(0, 0, idx);
	m_bIsPublishing = false;
	StreamPtr pStream = m_pChatRoom->removeStream(streamID);
	FreeAVView(pStream);
    
    m_strPublishStreamID = "";
	m_mainLiveView->setViewStreamID(m_strPublishStreamID);
	m_AuxLiveView->setViewStreamID("");
}

#if (defined Q_OS_WIN) && (defined USE_SURFACE_MERGE) 
void ZegoSingleAnchorDialog::StartSurfaceMerge()
{
	int cx = m_pAVSettings->GetResolution().cx;
	int cy = m_pAVSettings->GetResolution().cy;

	SurfaceMerge::SetFPS(m_pAVSettings->GetFps());
	SurfaceMerge::SetCursorVisible(true);
	SurfaceMerge::SetSurfaceSize(cx, cy);

	SurfaceMerge::ZegoCaptureItem *itemList = new SurfaceMerge::ZegoCaptureItem[2];

	SurfaceMerge::ZegoCaptureItem itemCam;
	strcpy(itemCam.captureSource.deviceId, m_pAVSettings->GetCameraId().toStdString().c_str());
	itemCam.captureType = SurfaceMerge::CaptureType::Camera;
	itemCam.position = { cx - cx / 4, cy - cy / 4, cx / 4, cy / 4 };  //摄像头默认置于右下角

	unsigned int count = 0;
	SurfaceMerge::ScreenItem *screenList = SurfaceMerge::EnumScreenList(count);
	SurfaceMerge::ZegoCaptureItem itemWin;
	for (int i = 0; i < count; i++)
	{
		if (screenList[i].bPrimary)
		{
			strcpy(itemWin.captureSource.screenName, screenList[i].szName);
			break;
		}
	}

	itemWin.captureType = SurfaceMerge::CaptureType::Screen;
	itemWin.position = { 0, 0, cx, cy };
	itemList[0] = itemCam;
	itemList[1] = itemWin;

	SurfaceMerge::UpdateSurface(itemList, 2);
	AVViews.last()->setSurfaceMergeView(true);
	AVViews.last()->setSurfaceMergeItemRect(itemWin, itemCam);
	SurfaceMerge::SetRenderView((void *)AVViews.last()->winId());

	delete[]itemList;
	SurfaceMerge::FreeScreenList(screenList);
}
#endif

void ZegoSingleAnchorDialog::GetOut()
{
	//离开房间时先把混音功能和声卡采集关闭
#ifdef Q_OS_WIN
	if (isUseDefaultAux)
		EndAux();
	else
	{
		AUDIOHOOK::StopAudioRecord();
		LIVEROOM::EnableAux(false);
		AUDIOHOOK::UnInitAudioHook();
	}
#else
	EndAux();
#endif

	if (ui.m_bCapture->text() == QStringLiteral("停止采集"))
#ifdef Q_OS_WIN
		LIVEROOM::EnableMixSystemPlayout(false);
#endif

	StopPublishStream(m_strPublishStreamID);
	if (m_pAVSettings->GetUsePublish2Stream())
	StopPublishStream(m_strPublishStreamID_Aux, ZEGO::AV::PUBLISH_CHN_AUX);

	roomMemberDelete(m_strCurUserName);
	LIVEROOM::LogoutRoom();
	if (timer != nullptr)
	    timer->stop();

	//释放堆内存
	delete m_cbMircoPhoneListView;
	delete m_cbCameraListView;
	delete m_memberModel;
	delete m_chatModel;
	delete m_cbMircoPhoneModel;
	delete m_cbCameraModel;
	delete timer;
	//指针置空
	m_cbMircoPhoneListView = nullptr;
	m_cbCameraListView = nullptr;
	m_memberModel = nullptr;
	m_chatModel = nullptr;
	m_cbMircoPhoneModel = nullptr;
	m_cbCameraModel = nullptr;
	timer = nullptr;

}

void ZegoSingleAnchorDialog::initComboBox()
{

	m_cbMircoPhoneModel = new QStringListModel(this);

	m_cbMircoPhoneModel->setStringList(m_MircoPhoneList);

	m_cbMircoPhoneListView = new QListView(this);
	ui.m_cbMircoPhone->setView(m_cbMircoPhoneListView);
	ui.m_cbMircoPhone->setModel(m_cbMircoPhoneModel);
	ui.m_cbMircoPhone->setItemDelegate(new NoFocusFrameDelegate(this));

	m_cbCameraModel = new QStringListModel(this);

	m_cbCameraModel->setStringList(m_CameraList);

	m_cbCameraListView = new QListView(this);
	ui.m_cbCamera->setView(m_cbCameraListView);
	ui.m_cbCamera->setModel(m_cbCameraModel);
	ui.m_cbCamera->setItemDelegate(new NoFocusFrameDelegate(this));

	m_cbCameraListView2 = new QListView(this);
	ui.m_cbCamera2->setView(m_cbCameraListView2);
	ui.m_cbCamera2->setModel(m_cbCameraModel);
	ui.m_cbCamera2->setItemDelegate(new NoFocusFrameDelegate(this));
}

void ZegoSingleAnchorDialog::EnumVideoAndAudioDevice()
{
#ifdef Q_OS_WIN
	//设备数
	int nDeviceCount = 0;
	AV::DeviceInfo* pDeviceList(NULL);

	//获取音频设备
	int curSelectionIndex = 0;
	pDeviceList = LIVEROOM::GetAudioDeviceList(AV::AudioDeviceType::AudioDevice_Input, nDeviceCount);
	for (int i = 0; i < nDeviceCount; ++i)
	{
		insertStringListModelItem(m_cbMircoPhoneModel, QString::fromUtf8(pDeviceList[i].szDeviceName), m_cbMircoPhoneModel->rowCount());
		m_vecAudioDeviceIDs.push_back(pDeviceList[i].szDeviceId);

		if (m_pAVSettings->GetMircophoneId() == QString(pDeviceList[i].szDeviceId))
			curSelectionIndex = i;
	}

	ui.m_cbMircoPhone->setCurrentIndex(curSelectionIndex);

	qDebug() << "[SingleAnchorDialog::EnumAudioDevice]: current audio device : " << m_pAVSettings->GetMircophoneId();
	LIVEROOM::FreeDeviceList(pDeviceList);

	pDeviceList = NULL;

	//获取视频设备
	curSelectionIndex = 0;
	pDeviceList = LIVEROOM::GetVideoDeviceList(nDeviceCount);
	for (int i = 0; i < nDeviceCount; ++i)
	{
		insertStringListModelItem(m_cbCameraModel, QString::fromUtf8(pDeviceList[i].szDeviceName), m_cbCameraModel->rowCount());
		m_vecVideoDeviceIDs.push_back(pDeviceList[i].szDeviceId);

		if (m_pAVSettings->GetCameraId() == QString(pDeviceList[i].szDeviceId))
			curSelectionIndex = i;
	}

	ui.m_cbCamera->blockSignals(true);
	ui.m_cbCamera->setCurrentIndex(curSelectionIndex);
	ui.m_cbCamera->blockSignals(false);
	qDebug() << "[SingleAnchorDialog::EnumVideoDevice]: current video device_main : " << m_pAVSettings->GetCameraId();

	curSelectionIndex = -1;
	for (int i = 0; i < nDeviceCount; ++i)
	{
		if (m_pAVSettings->GetCameraId2() == QString(pDeviceList[i].szDeviceId))
			curSelectionIndex = i;
	}

	if (curSelectionIndex < 0)
	{
		//先将第二个camera model用一个空的model绑定，此时就算推了第二路流也会没有图像
		ui.m_cbCamera2->blockSignals(true);
		ui.m_cbCamera2->setModel(new QStringListModel(this));
		ui.m_cbCamera2->blockSignals(false);
	}
	else
	{
		ui.m_cbCamera2->blockSignals(true);
		ui.m_cbCamera2->setCurrentIndex(curSelectionIndex);
		ui.m_cbCamera2->blockSignals(false);
	}

	qDebug() << "[SingleAnchorDialog::EnumVideoDevice]: current video device_aux : " << m_pAVSettings->GetCameraId2();
	LIVEROOM::FreeDeviceList(pDeviceList);
	pDeviceList = NULL;
#else
	QVector<deviceConfig> audioDeviceList = GetAudioDevicesWithOSX();
	QVector<deviceConfig> videoDeviceList = GetVideoDevicesWithOSX();

	//将从mac系统API中获取的Audio设备保存
	int curSelectionIndex = 0;
	for (int i = 0; i < audioDeviceList.size(); ++i)
	{
		insertStringListModelItem(m_cbMircoPhoneModel, audioDeviceList[i].deviceName, m_cbMircoPhoneModel->rowCount());
		m_vecAudioDeviceIDs.push_back(audioDeviceList[i].deviceId);

		if (m_pAVSettings->GetMircophoneId() == audioDeviceList[i].deviceId)
			curSelectionIndex = i;
	}

	ui.m_cbMircoPhone->setCurrentIndex(curSelectionIndex);

	//将从mac系统API中获取的Video设备保存
	curSelectionIndex = 0;
	for (int i = 0; i < videoDeviceList.size(); ++i)
	{
		insertStringListModelItem(m_cbCameraModel, videoDeviceList[i].deviceName, m_cbCameraModel->rowCount());
		m_vecVideoDeviceIDs.push_back(videoDeviceList[i].deviceId);

		if (m_pAVSettings->GetCameraId() == videoDeviceList[i].deviceId)
			curSelectionIndex = i;
	}

	ui.m_cbCamera->setCurrentIndex(curSelectionIndex);
#endif
}

void ZegoSingleAnchorDialog::insertStringListModelItem(QStringListModel * model, QString name, int size)
{
	if (model == nullptr)
		return;

	int row = size;
	model->insertRows(row, 1);
	QModelIndex index = model->index(row);
	model->setData(index, name);

}

void ZegoSingleAnchorDialog::removeStringListModelItem(QStringListModel * model, QString name)
{
	if (model == nullptr)
		return;

	if (model->rowCount() > 0)
	{
		int curIndex = -1;
		QStringList list = model->stringList();
		for (int i = 0; i < list.size(); i++)
		{
			if (list[i] == name)
				curIndex = i;
		}

		model->removeRows(curIndex, 1);
	}

}

int ZegoSingleAnchorDialog::takeLeastAvaliableViewIndex()
{
	int min = m_avaliableView[0];
	int minIndex = 0;
	for (int i = 1; i < m_avaliableView.size(); i++)
	{
		if (m_avaliableView[i] < min)
		{
			min = m_avaliableView[i];
			minIndex = i;
		}
	}

	m_avaliableView.takeAt(minIndex);
	return min;
}

void ZegoSingleAnchorDialog::FreeAVView(StreamPtr stream)
{
	if (stream == nullptr)
	{
		return;
	}

	int nIndex = stream->getPlayView();

	m_avaliableView.push_front(nIndex);

	//刷新可用的view页面
	update();
}

QString ZegoSingleAnchorDialog::encodeStringAddingEscape(QString str)
{
	for (int i = 0; i < str.size(); i++)
	{
		if (str.at(i) == '!'){
			str.replace(i, 1, "%21");
			i += 2;
		}
		else if (str.at(i) == '*'){
			str.replace(i, 1, "%2A");
			i += 2;
		}
		else if (str.at(i) == '\''){
			str.replace(i, 1, "%27");
			i += 2;
		}
		else if (str.at(i) == '('){
			str.replace(i, 1, "%28");
			i += 2;
		}
		else if (str.at(i) == ')'){
			str.replace(i, 1, "%29");
			i += 2;
		}
		else if (str.at(i) == ';'){
			str.replace(i, 1, "%3B");
			i += 2;
		}
		else if (str.at(i) == ':'){
			str.replace(i, 1, "%3A");
			i += 2;
		}
		else if (str.at(i) == '@'){
			str.replace(i, 1, "%40");
			i += 2;
		}
		else if (str.at(i) == '&'){
			str.replace(i, 1, "%26");
			i += 2;
		}
		else if (str.at(i) == '='){
			str.replace(i, 1, "%3D");
			i += 2;
		}
		else if (str.at(i) == '+'){
			str.replace(i, 1, "%2B");
			i += 2;
		}
		else if (str.at(i) == '$'){
			str.replace(i, 1, "%24");
			i += 2;
		}
		else if (str.at(i) == ','){
			str.replace(i, 1, "%2C");
			i += 2;
		}
		else if (str.at(i) == '/'){
			str.replace(i, 1, "%2F");
			i += 2;
		}
		else if (str.at(i) == '?'){
			str.replace(i, 1, "%2A");
			i += 2;
		}
		else if (str.at(i) == '%'){
			str.replace(i, 1, "%25");
			i += 2;
		}
		else if (str.at(i) == '#'){
			str.replace(i, 1, "%23");
			i += 2;
		}
		else if (str.at(i) == '['){
			str.replace(i, 1, "%5B");
			i += 2;
		}
		else if (str.at(i) == ']'){
			str.replace(i, 1, "%5D");
			i += 2;
		}
	}
	return str;
}

void ZegoSingleAnchorDialog::roomMemberAdd(QString userName)
{
	if (m_memberModel == nullptr)
		return;

	insertStringListModelItem(m_memberModel, userName, m_memberModel->rowCount());
	ui.m_tabCommonAndUserList->setTabText(1, QString(QStringLiteral("成员(%1)").arg(m_memberModel->rowCount())));
}

void ZegoSingleAnchorDialog::roomMemberDelete(QString userName)
{
	if (m_memberModel == nullptr)
		return;

	removeStringListModelItem(m_memberModel, userName);
	ui.m_tabCommonAndUserList->setTabText(1, QString(QStringLiteral("成员(%1)").arg(m_memberModel->rowCount())));
}

void ZegoSingleAnchorDialog::BeginAux()
{
	QString filePath = QFileDialog::getOpenFileName(this,
		tr(QStringLiteral("请选择一个混音文件").toStdString().c_str()),
		"./Resources",
		tr(QStringLiteral("pcm文件(*.pcm)").toStdString().c_str()));


	if (!filePath.isEmpty())
	{
		FILE* fAux;
		fAux = fopen(filePath.toStdString().c_str(), "rb");

		if (fAux == NULL)
		{
			QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("文件内容错误: %1").arg(filePath));
			return;
		}

		fseek(fAux, 0, SEEK_END);
		m_nAuxDataLen = ftell(fAux);

		if (m_nAuxDataLen > 0)
		{
			m_pAuxData = new unsigned char[m_nAuxDataLen];
			memset(m_pAuxData, 0, m_nAuxDataLen);
		}

		fseek(fAux, 0, 0);

		int nReadDataLen = fread(m_pAuxData, sizeof(unsigned char), m_nAuxDataLen, fAux);

		fclose(fAux);

		LIVEROOM::EnableAux(true);

	}
}

void ZegoSingleAnchorDialog::EndAux()
{
	LIVEROOM::EnableAux(false);

	if (m_pAuxData)
	{
		delete[] m_pAuxData;
		m_pAuxData = NULL;
	}
	m_nAuxDataLen = 0;
	m_nAuxDataPos = 0;
}

void ZegoSingleAnchorDialog::setWaterPrint(AV::PublishChannelIndex idx)
{
	QString waterPrintPath = QDir::currentPath();
	waterPrintPath += "/Resources/images/";
	if (m_dpi < 2.0)
	{
		waterPrintPath += "waterprint.png";
	}
	else
	{
		waterPrintPath += "@2x/waterprint@2x.png";
	}

	QImage waterPrint(waterPrintPath);

	//标准640 * 360，根据标准对当前分辨率的水印进行等比缩放
	int cx = m_pAVSettings->GetResolution().cx;
	int cy = m_pAVSettings->GetResolution().cy;
	float scaleX = cx * 1.0 / 640;
	float scaleY = cy * 1.0 / 360;
	
	if (idx == ZEGO::AV::PUBLISH_CHN_MAIN)
	{
		LIVEROOM::SetPublishWaterMarkRect((int)(20 * scaleX), (int)(20 * scaleY), (int)(123 * scaleX), (int)(69 * scaleY));
		LIVEROOM::SetPreviewWaterMarkRect((int)(20 * scaleX), (int)(20 * scaleY), (int)(123 * scaleX), (int)(69 * scaleY));
		LIVEROOM::SetWaterMarkImagePath(waterPrintPath.toStdString().c_str());
	}
	else
	{
		LIVEROOM::SetPublishWaterMarkRect((int)(20 * scaleX), (int)(20 * scaleY), (int)(123 * scaleX), (int)(69 * scaleY), ZEGO::AV::PUBLISH_CHN_AUX);
		LIVEROOM::SetPreviewWaterMarkRect((int)(20 * scaleX), (int)(20 * scaleY), (int)(123 * scaleX), (int)(69 * scaleY), ZEGO::AV::PUBLISH_CHN_AUX);
		LIVEROOM::SetWaterMarkImagePath(waterPrintPath.toStdString().c_str(), ZEGO::AV::PUBLISH_CHN_AUX);
	}
}

int ZegoSingleAnchorDialog::getCameraIndexFromID(const QString& cameraID)
{
	for (int i = 0; i < m_vecVideoDeviceIDs.size(); ++i)
	{
		if (m_vecVideoDeviceIDs[i] == cameraID)
			return i;
	}

	return -1;
}

//SDK回调
void ZegoSingleAnchorDialog::OnLoginRoom(int errorCode, const QString& strRoomID, QVector<StreamPtr> vStreamList)
{
	qDebug() << "Login Room!";
	if (errorCode != 0)
	{
		QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("登陆房间失败,错误码: %1").arg(errorCode));
		OnClose();
		return;
	}

	//加入房间列表
	roomMemberAdd(m_strCurUserName);

	StartPublishStream();
	
}


void ZegoSingleAnchorDialog::OnPublishStateUpdate(int stateCode, const QString& streamId, StreamPtr streamInfo)
{
	qDebug() << "Publish success!";
	if (stateCode == 0)
	{
		if (streamInfo != nullptr)
		{
		    m_anchorStreamInfo = streamInfo;
			sharedHlsUrl = streamInfo->m_vecHlsUrls[0];
			sharedRtmpUrl = streamInfo->m_vecRtmpUrls[0];
			

			QString strUrl;

			QString strRtmpUrl = (streamInfo->m_vecRtmpUrls.size() > 0) ?
				streamInfo->m_vecRtmpUrls[0] : QStringLiteral("");

			if (!strRtmpUrl.isEmpty())
			{
				strUrl.append(QStringLiteral("1. "));
				strUrl.append(strRtmpUrl);
				strUrl.append(QStringLiteral("\r\n"));
			}

			QString strFlvUrl = (streamInfo->m_vecFlvUrls.size() > 0) ?
				streamInfo->m_vecFlvUrls[0] : QStringLiteral("");

			if (!strFlvUrl.isEmpty())
			{
				strUrl.append(QStringLiteral("2. "));
				strUrl.append(strFlvUrl);
				strUrl.append(QStringLiteral("\r\n"));
			}

			QString strHlsUrl = (streamInfo->m_vecHlsUrls.size() > 0) ?
				streamInfo->m_vecHlsUrls[0] : QStringLiteral("");

			if (!strHlsUrl.isEmpty())
			{
				strUrl.append(QStringLiteral("3. "));
				strUrl.append(strHlsUrl);
				strUrl.append(QStringLiteral("\r\n"));
			}

		}

		//在单主播模式下，推流成功时需将流媒体地址存到流附加信息中
		if (sharedHlsUrl.size() > 0 && sharedRtmpUrl.size() > 0)
		{
			//封装存放分享地址的json对象
			QMap<QString, QString> mapUrls = QMap<QString, QString>();

			mapUrls.insert(m_FirstAnchor, QString::number(true));
			mapUrls.insert(m_HlsKey, sharedHlsUrl);
			mapUrls.insert(m_RtmpKey, sharedRtmpUrl);

			QVariantMap vMap;
			QMapIterator<QString, QString> it(mapUrls);
			while (it.hasNext())
			{
				it.next();
				vMap.insert(it.key(), it.value());
			}

			QJsonDocument doc = QJsonDocument::fromVariant(vMap);
			QByteArray jba = doc.toJson();
			QString jsonString = QString(jba);
			//设置流附加消息，将混流信息传入
			LIVEROOM::SetPublishStreamExtraInfo(jsonString.toStdString().c_str());
		}

		ui.m_bAux->setEnabled(true);
		ui.m_bCapture->setEnabled(true);
		ui.m_bShare->setEnabled(true);
		ui.m_bRequestJoinLive->setEnabled(true);
		ui.m_cbMircoPhone->setEnabled(true);
		ui.m_cbCamera->setEnabled(true);
		if (m_pAVSettings->GetUsePublish2Stream())
		ui.m_cbCamera2->setEnabled(true);
		ui.m_bRequestJoinLive->setText(QStringLiteral("停止直播"));

		//推流成功后启动计时器监听麦克风音量
		timer->start(0);
		
	}
	else
	{
		QMessageBox::warning(NULL, QStringLiteral("推流失败"), QStringLiteral("错误码: %1").arg(stateCode));
		ui.m_bRequestJoinLive->setText(QStringLiteral("开始直播"));
		ui.m_bRequestJoinLive->setEnabled(true);

		EndAux();
		// 停止预览, 回收view
		LIVEROOM::StopPreview();
		LIVEROOM::SetPreviewView(nullptr);
		StreamPtr pStream = m_pChatRoom->removeStream(streamId);
		FreeAVView(pStream);
	}
}


void ZegoSingleAnchorDialog::OnUserUpdate(QVector<QString> userIDs, QVector<QString> userNames, QVector<int> userFlags, QVector<int> userRoles, unsigned int userCount, LIVEROOM::ZegoUserUpdateType type)
{
	qDebug() << "onUserUpdate!";

	//全量更新
	if (type == LIVEROOM::ZegoUserUpdateType::UPDATE_TOTAL){
		//removeAll
		m_memberModel->removeRows(0, m_memberModel->rowCount());

		insertStringListModelItem(m_memberModel, m_strCurUserName, 0);
		for (int i = 0; i < userCount; i++)
		{
			insertStringListModelItem(m_memberModel, userNames[i], m_memberModel->rowCount());
		}
	}
	//增量更新
	else
	{

		for (int i = 0; i < userCount; i++)
		{

			if (userFlags[i] == LIVEROOM::USER_ADDED)
				insertStringListModelItem(m_memberModel, userNames[i], m_memberModel->rowCount());
			else
				removeStringListModelItem(m_memberModel, userNames[i]);
		}
	}

	ui.m_tabCommonAndUserList->setTabText(1, QString(QStringLiteral("成员(%1)").arg(m_memberModel->rowCount())));
	ui.m_listMember->update();
}

void ZegoSingleAnchorDialog::OnDisconnect(int errorCode, const QString& roomId)
{
	if (m_pChatRoom->getRoomId() == roomId)
	{
		QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("您已掉线"));
		OnClose();
	}
}

void ZegoSingleAnchorDialog::OnKickOut(int reason, const QString& roomId)
{
	if (m_pChatRoom->getRoomId() == roomId)
	{
		QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("您已被踢出房间"));
		OnClose();
	}
}


void ZegoSingleAnchorDialog::OnPublishQualityUpdate(const QString& streamId, int quality, double videoFPS, double videoKBS)
{
	StreamPtr pStream = m_pChatRoom->getStreamById(streamId);

	if (pStream == nullptr)
		return;

	int nIndex = pStream->getPlayView();

	if (nIndex < 0 || nIndex > 11)
		return;

	AVViews[nIndex]->setCurrentQuality(quality);

	//QVector<QString> q = { QStringLiteral("优"), QStringLiteral("良"), QStringLiteral("中"), QStringLiteral("差") };
	//qDebug() << QStringLiteral("当前窗口") << nIndex << QStringLiteral("的直播质量为") << q[quality];

}

void ZegoSingleAnchorDialog::OnAVAuxInput(unsigned char *pData, int *pDataLen, int pDataLenValue, int *pSampleRate, int *pNumChannels)
{
#ifdef Q_OS_WIN
	if (isUseDefaultAux)
	{
		if (m_pAuxData != nullptr && (*pDataLen < m_nAuxDataLen))
		{
			*pSampleRate = 44100;
			*pNumChannels = 2;

			if (m_nAuxDataPos + *pDataLen > m_nAuxDataLen)
			{
				m_nAuxDataPos = 0;
			}

			int nCopyLen = *pDataLen;
			memcpy(pData, m_pAuxData + m_nAuxDataPos, nCopyLen);

			m_nAuxDataPos += *pDataLen;

			*pDataLen = nCopyLen;


		}
		else
		{
			*pDataLen = 0;
		}
	}
	else
	{
		AUDIOHOOK::GetAUXData(pData, pDataLen, pSampleRate, pNumChannels);
	}
#else
	if (m_pAuxData != nullptr && (*pDataLen < m_nAuxDataLen))
	{
		*pSampleRate = 44100;
		*pNumChannels = 2;

		if (m_nAuxDataPos + *pDataLen > m_nAuxDataLen)
		{
			m_nAuxDataPos = 0;
		}

		int nCopyLen = *pDataLen;
		memcpy(pData, m_pAuxData + m_nAuxDataPos, nCopyLen);

		m_nAuxDataPos += *pDataLen;

		*pDataLen = nCopyLen;


	}
	else
	{
		*pDataLen = 0;
	}
#endif
}

void ZegoSingleAnchorDialog::OnSendRoomMessage(int errorCode, const QString& roomID, int sendSeq, unsigned long long messageId)
{
	if (errorCode != 0) 
	{
		QMessageBox::warning(NULL, QStringLiteral("消息发送失败"), QStringLiteral("错误码: %1").arg(errorCode));
		return; 
	}
     
	qDebug() << "message send success";
}

void ZegoSingleAnchorDialog::OnRecvRoomMessage(const QString& roomId, QVector<RoomMsgPtr> vRoomMsgList)
{
	for (auto& roomMsg : vRoomMsgList)
	{
		QString strTmpContent;
		strTmpContent = QString(QStringLiteral("%1")).arg(roomMsg->getContent());
		qDebug() << strTmpContent;

		QStandardItem *item = new QStandardItem;
		m_chatModel->appendRow(item);
		QModelIndex index = m_chatModel->indexFromItem(item);

		ZegoRoomMessageLabel *chatContent = new ZegoRoomMessageLabel;
		chatContent->setTextContent(roomMsg->getUserName(), strTmpContent);

		ui.m_listChat->setIndexWidget(index, chatContent);
		if (chatContent->getHeightNum() > 1)
			ui.m_listChat->resizeRowToContents(m_chatModel->rowCount() - 1);

		//每次接受消息均显示最后一条
		ui.m_listChat->scrollToBottom();

	}
}


void ZegoSingleAnchorDialog::OnAudioDeviceChanged(AV::AudioDeviceType deviceType, const QString& strDeviceId, const QString& strDeviceName, AV::DeviceState state)
{
	if (deviceType == AV::AudioDeviceType::AudioDevice_Output)
		return;

	if (state == AV::DeviceState::Device_Added)
	{
		insertStringListModelItem(m_cbMircoPhoneModel, strDeviceName, m_cbMircoPhoneModel->rowCount());
		m_vecAudioDeviceIDs.push_back(strDeviceId);
		if (m_vecAudioDeviceIDs.size() == 1)
		{
			LIVEROOM::SetAudioDevice(AV::AudioDevice_Input, m_vecAudioDeviceIDs[0].toStdString().c_str());
			m_pAVSettings->SetMicrophoneId(m_vecAudioDeviceIDs[0]);
			ui.m_cbMircoPhone->setCurrentIndex(0);
		}
		update();
	}
	else if (state == AV::DeviceState::Device_Deleted)
	{
		for (int i = 0; i < m_vecAudioDeviceIDs.size(); i++)
		{
			if (m_vecAudioDeviceIDs[i] != strDeviceId)
				continue;

			m_vecAudioDeviceIDs.erase(m_vecAudioDeviceIDs.begin() + i);


			int currentCurl = ui.m_cbMircoPhone->currentIndex();
			//如果currentCurl等于i说明当前删除的设备是当前正在使用的设备
			if (currentCurl == i)
			{
				//如果删除之后还有能播放的设备存在，则默认选择第一个音频设备
				if (m_vecAudioDeviceIDs.size() > 0)
				{
					LIVEROOM::SetAudioDevice(AV::AudioDevice_Input, m_vecAudioDeviceIDs[0].toStdString().c_str());
					m_pAVSettings->SetMicrophoneId(m_vecAudioDeviceIDs[0]);
					ui.m_cbMircoPhone->setCurrentIndex(0);

				}
				else
				{
					LIVEROOM::SetAudioDevice(AV::AudioDevice_Input, NULL);
					m_pAVSettings->SetMicrophoneId("");
				}
				removeStringListModelItem(m_cbMircoPhoneModel, strDeviceName);
				update();
				break;
			}


		}
	}
}

void ZegoSingleAnchorDialog::OnVideoDeviceChanged(const QString& strDeviceId, const QString& strDeviceName, AV::DeviceState state)
{
	if (state == AV::DeviceState::Device_Added)
	{
		insertStringListModelItem(m_cbCameraModel, strDeviceName, m_cbCameraModel->rowCount());
		m_vecVideoDeviceIDs.push_back(strDeviceId);
		if (m_vecVideoDeviceIDs.size() == 1)
		{
			LIVEROOM::StopPreview();
			LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[0].toStdString().c_str());
			LIVEROOM::StartPreview();

			m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[0]);
			ui.m_cbCamera->blockSignals(true);
			ui.m_cbCamera->setCurrentIndex(0);
			ui.m_cbCamera->blockSignals(false);

			ui.m_cbCamera->blockSignals(true);
			ui.m_cbCamera2->setModel(new QStringListModel(this));
			ui.m_cbCamera->blockSignals(false);

		}
		//当前摄像头有两个的话，将最新加入的摄像头分配给摄像头2
		else if (m_vecVideoDeviceIDs.size() == 2)
		{
			LIVEROOM::StopPreview(ZEGO::AV::PUBLISH_CHN_AUX);
			LIVEROOM::SetVideoDevice(strDeviceId.toStdString().c_str(), ZEGO::AV::PUBLISH_CHN_AUX);
			
			m_pAVSettings->SetCameraId2(strDeviceId);

			ui.m_cbCamera2->blockSignals(true);
			ui.m_cbCamera2->setModel(m_cbCameraModel);
			ui.m_cbCamera2->setCurrentIndex(1);
			ui.m_cbCamera2->blockSignals(false);

			ui.m_cbCamera->blockSignals(true);
			ui.m_cbCamera->setCurrentIndex(0);
			ui.m_cbCamera->blockSignals(false);
			m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[0]);

			LIVEROOM::EnableCamera(true, ZEGO::AV::PUBLISH_CHN_AUX);
			LIVEROOM::StartPreview(ZEGO::AV::PUBLISH_CHN_AUX);
		}
		update();

		qDebug() << "[SingleAnchorDialog::VideoDevice_Added]: current video device_main : " << m_pAVSettings->GetCameraId() << " video device_aux : " << m_pAVSettings->GetCameraId2();
	}
	else if (state == AV::DeviceState::Device_Deleted)
	{
		for (int i = 0; i < m_vecVideoDeviceIDs.size(); i++)
		{
			if (m_vecVideoDeviceIDs[i] != strDeviceId)
				continue;

			int currentCurl_1 = ui.m_cbCamera->currentIndex();
			int currentCurl_2 = ui.m_cbCamera2->currentIndex();
			//如果currentCurl等于i说明当前删除的设备是当前正在使用的设备1
			if (currentCurl_1 == i)
			{
				//默认采集靠前的没有被使用的视频设备
				if (m_vecVideoDeviceIDs.size() > 1)
				{
					int index = -1;
					for (int j = 0; j < m_vecVideoDeviceIDs.size(); ++j)
					{
						if ((j != currentCurl_1) && (j != currentCurl_2))
						{
							index = j;
							break;
						}
					}
					//这种情况是摄像头2占了剩下的一个设备，这个时候需将该设备给1
					if (index < 0)
					{
						LIVEROOM::EnableCamera(false, ZEGO::AV::PUBLISH_CHN_AUX);
						LIVEROOM::StopPreview(ZEGO::AV::PUBLISH_CHN_AUX);

						LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[currentCurl_2].toStdString().c_str());
						m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[currentCurl_2]);
						ui.m_cbCamera->blockSignals(true);
						ui.m_cbCamera->setCurrentIndex(currentCurl_2);
						ui.m_cbCamera->blockSignals(false);

						ui.m_cbCamera2->blockSignals(true);
						ui.m_cbCamera2->setModel(new QStringListModel(this));
						ui.m_cbCamera2->blockSignals(false);

						m_pAVSettings->SetCameraId2("");
					}
					else
					{
						LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[index].toStdString().c_str());
						m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[index]);
						ui.m_cbCamera->blockSignals(true);
						ui.m_cbCamera->setCurrentIndex(index);
						ui.m_cbCamera->blockSignals(false);
					}

				}
				else if (m_vecVideoDeviceIDs.size() <= 1)
				{
					LIVEROOM::SetVideoDevice(nullptr);
					m_pAVSettings->SetCameraId("");

				}

			}
			else if (currentCurl_2 == i)
			{
				if (m_vecVideoDeviceIDs.size() > 1)
				{
					int index = -1;
					for (int j = 0; j < m_vecVideoDeviceIDs.size(); ++j)
					{
						if ((j != currentCurl_1) && (j != currentCurl_2))
						{
							index = j;
							break;
						}
					}
					//这种情况是摄像头1占了剩下的一个设备，这个时候设备2直接复制空
					if (index < 0)
					{
						LIVEROOM::EnableCamera(false, ZEGO::AV::PUBLISH_CHN_AUX);
						LIVEROOM::SetVideoDevice(nullptr, ZEGO::AV::PUBLISH_CHN_AUX);
						ui.m_cbCamera2->blockSignals(true);
						ui.m_cbCamera2->setModel(new QStringListModel(this));
						ui.m_cbCamera2->blockSignals(false);
						m_pAVSettings->SetCameraId2("");
					}
					else
					{
						LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[index].toStdString().c_str(), ZEGO::AV::PUBLISH_CHN_AUX);
						m_pAVSettings->SetCameraId2(m_vecVideoDeviceIDs[index]);
						ui.m_cbCamera2->blockSignals(true);
						ui.m_cbCamera2->setCurrentIndex(index);
						ui.m_cbCamera2->blockSignals(false);
					}
				}
			}

			m_vecVideoDeviceIDs.erase(m_vecVideoDeviceIDs.begin() + i);
			ui.m_cbCamera->blockSignals(true);
			ui.m_cbCamera2->blockSignals(true);
			removeStringListModelItem(m_cbCameraModel, strDeviceName);
			ui.m_cbCamera->blockSignals(false);
			ui.m_cbCamera2->blockSignals(false);
			update();
			break;
		}

	qDebug() << "[SingleAnchorDialog::VideoDevice_Deleted]: current video device_main : " << m_pAVSettings->GetCameraId() << " video device_aux : " << m_pAVSettings->GetCameraId2();
	}
}

void ZegoSingleAnchorDialog::OnPreviewSnapshot(void *pImage)
{
	QImage *image;

#ifdef Q_OS_WIN
	image = new QImage;
	QPixmap pixmap = qt_pixmapFromWinHBITMAP((HBITMAP)pImage, 0);
	*image = pixmap.toImage();
#else
	image = CGImageToQImage(pImage);
#endif

	//发送信号切线程，不能阻塞当前线程
	emit sigShowSnapShotImage(image);
}


#if (defined Q_OS_WIN) && (defined USE_SURFACE_MERGE)
void ZegoSingleAnchorDialog::OnSurfaceMergeResult(unsigned char *surfaceMergeData, int datalength)
{
	if (m_takeSnapShot)
	{
		//同一时刻只允许一个线程进入该代码段
		m_takeSnapShot = false;
		m_mutex.lock();
	    m_image = new unsigned char[datalength];
		memcpy(m_image, surfaceMergeData, datalength);
		QImage *image = new QImage(m_image, m_pAVSettings->GetResolution().cx, m_pAVSettings->GetResolution().cy, QImage::Format_RGB32);
		emit sigShowSnapShotImage(image);
		m_mutex.unlock();
	}
}

#endif

//UI回调
void ZegoSingleAnchorDialog::OnClickTitleButton()
{
	QPushButton *pButton = qobject_cast<QPushButton *>(sender());

	if (pButton == ui.m_bMin)
	{
		this->showMinimized();
	}
	else if (pButton == ui.m_bClose)
	{
		this->close();
	}
	else if (pButton == ui.m_bMax)
	{
		if (isMax)
		{
			this->showNormal();
			isMax = false;
		}
		else
		{
			this->showMaximized();
			isMax = true;
		}
	}
}

void ZegoSingleAnchorDialog::OnButtonSwitchPublish()
{
	//当前按钮文本为“开始直播”
	if (ui.m_bRequestJoinLive->text() == QStringLiteral("开始直播"))
	{
		ui.m_bRequestJoinLive->setText(QStringLiteral("开启中..."));
		ui.m_bRequestJoinLive->setEnabled(false);
		//开始推流
		StartPublishStream();

	}
	//当前按钮文本为“停止直播”
	else
	{
		ui.m_bRequestJoinLive->setText(QStringLiteral("停止中..."));
		ui.m_bRequestJoinLive->setEnabled(false);

		StopPublishStream(m_strPublishStreamID);
		if (m_pAVSettings->GetUsePublish2Stream())
		StopPublishStream(m_strPublishStreamID_Aux, ZEGO::AV::PUBLISH_CHN_AUX);

		if (ui.m_bAux->text() == QStringLiteral("关闭混音"))
		{
			ui.m_bAux->setText(QStringLiteral("关闭中..."));
			ui.m_bAux->setEnabled(false);

#ifdef Q_OS_WIN
			if (isUseDefaultAux)
			{
				EndAux();

			}
			else
			{
				AUDIOHOOK::StopAudioRecord();
				LIVEROOM::EnableAux(false);
				AUDIOHOOK::UnInitAudioHook();

			}
#else
			EndAux();
#endif
			ui.m_bAux->setText(QStringLiteral("开启混音"));
		}
		//停止直播后不能混音、声音采集、分享
		ui.m_bAux->setEnabled(false);
		ui.m_bCapture->setEnabled(false);
		ui.m_bShare->setEnabled(false);
		ui.m_bRequestJoinLive->setEnabled(true);
		ui.m_cbMircoPhone->setEnabled(false);
		ui.m_cbCamera->setEnabled(false);
		ui.m_cbCamera2->setEnabled(false);
		ui.m_bRequestJoinLive->setText(QStringLiteral("开始直播"));
	}
}

void ZegoSingleAnchorDialog::OnClose()
{
	//GetOut();
	this->close();
	//emit sigSaveVideoSettings(m_pAVSettings);
	//m_lastDialog->show();
}

void ZegoSingleAnchorDialog::OnButtonSendMessage()
{
	QString strChat;
	strChat = ui.m_edInput->toPlainText();

	m_strLastSendMsg = strChat;
	m_strLastSendMsg =  m_strLastSendMsg.trimmed();
    
	if (!m_strLastSendMsg.isEmpty())
	LIVEROOM::SendRoomMessage(ROOM::ZegoMessageType::Text, ROOM::ZegoMessageCategory::Chat, ROOM::ZegoMessagePriority::Default, m_strLastSendMsg.toStdString().c_str());

	ui.m_edInput->setText(QStringLiteral(""));

	QString strTmpContent;
	strTmpContent = QString(QStringLiteral("%1")).arg(m_strLastSendMsg);

	QStandardItem *item = new QStandardItem;
	m_chatModel->appendRow(item);
	QModelIndex index = m_chatModel->indexFromItem(item);

	ZegoRoomMessageLabel *chatContent = new ZegoRoomMessageLabel;
	chatContent->setTextContent(QStringLiteral("我"), strTmpContent);

	ui.m_listChat->setIndexWidget(index, chatContent);
	if (chatContent->getHeightNum() > 1)
		ui.m_listChat->resizeRowToContents(m_chatModel->rowCount() - 1);
	

	//每次发送消息均显示最后一条
	ui.m_listChat->scrollToBottom();
	m_strLastSendMsg.clear();

}

void ZegoSingleAnchorDialog::OnButtonSoundCapture()
{
	if (ui.m_bCapture->text() == QStringLiteral("声卡采集"))
	{
#ifdef Q_OS_WIN
		LIVEROOM::EnableMixSystemPlayout(true);
#endif
		ui.m_bCapture->setText(QStringLiteral("停止采集"));
	}
	else
	{
#ifdef Q_OS_WIN
		LIVEROOM::EnableMixSystemPlayout(false);
#endif
		ui.m_bCapture->setText(QStringLiteral("声卡采集"));
	}
}

void ZegoSingleAnchorDialog::OnButtonMircoPhone()
{


	if (ui.m_bProgMircoPhone->isChecked())
	{
		m_bCKEnableMic = true;
		ui.m_bProgMircoPhone->setMyEnabled(true);
		timer->start(0);
	}
	else
	{
		m_bCKEnableMic = false;
		timer->stop();
		ui.m_bProgMircoPhone->setMyEnabled(false);
		ui.m_bProgMircoPhone->update();
	}

	//使用麦克风
	LIVEROOM::EnableMic(m_bCKEnableMic);
}

void ZegoSingleAnchorDialog::OnButtonSound()
{


	if (ui.m_bSound->isChecked())
	{

		m_bCKEnableSpeaker = true;
	}
	else
	{

		m_bCKEnableSpeaker = false;
	}
	
	//使用扬声器
	LIVEROOM::EnableSpeaker(m_bCKEnableSpeaker);

}

void ZegoSingleAnchorDialog::OnProgChange()
{
	ui.m_bProgMircoPhone->setProgValue(LIVEROOM::GetCaptureSoundLevel());
	ui.m_bProgMircoPhone->update();
}

void ZegoSingleAnchorDialog::OnShareLink()
{

	QString encodeHls = encodeStringAddingEscape(sharedHlsUrl);
	QString encodeRtmp = encodeStringAddingEscape(sharedRtmpUrl);
	QString encodeID = encodeStringAddingEscape(m_pChatRoom->getRoomId());
	QString encodeStreamID = encodeStringAddingEscape(m_anchorStreamInfo->getStreamId());

	QString link("http://www.zego.im/share/index2?video=" + encodeHls + "&rtmp=" + encodeRtmp + "&id=" + encodeID + "&stream=" + encodeStreamID);
	ZegoShareDialog share(link);
	share.exec();

}

void ZegoSingleAnchorDialog::OnButtonShowFullScreen()
{
	//直播窗口总在最顶层
	//ui.m_zoneLiveView_Inner->setWindowFlags(ui.m_zoneLiveView_Inner->windowFlags() | Qt::WindowStaysOnTopHint);
	//ui.m_zoneLiveView_Inner->setParent(NULL);
	//qDebug() << ui.m_zoneLiveView_Inner->windowFlags();
	ui.m_zoneLiveView_Inner->setWindowFlags(Qt::Dialog);
	ui.m_zoneLiveView_Inner->showFullScreen();
	m_isLiveFullScreen = true;
}

void ZegoSingleAnchorDialog::OnSnapshotPreview()
{
	if (m_pAVSettings->GetSurfaceMerge())
	{
		m_takeSnapShot = true;
	}
	else
	{
		LIVEROOM::TakeSnapshotPreview();
	}
}

void ZegoSingleAnchorDialog::OnUseDefaultAux(bool state)
{
	BeginAux();
	isUseDefaultAux = state;
	ui.m_bAux->setEnabled(true);
	ui.m_bAux->setText(QStringLiteral("关闭混音"));
}

#ifdef Q_OS_WIN
void ZegoSingleAnchorDialog::OnGetMusicAppPath(QString exePath)
{
	
	QString dllPath = QDir::currentPath() + "/MusicHook/ZegoMusicAudio.dll";

	const char*  exepath;
	QByteArray exeBA = exePath.toLocal8Bit();
	exepath = exeBA.data();

	const char*  dllpath;
	QByteArray dllBA = dllPath.toLocal8Bit();
	dllpath = dllBA.data();

	AUDIOHOOK::InitAuidoHook();
	if (!AUDIOHOOK::StartAudioHook(exepath, dllpath))
	{
		QMessageBox::warning(NULL, QStringLiteral("警告"), QStringLiteral("路径格式错误"));
		ui.m_bAux->setEnabled(true);
		ui.m_bAux->setText(QStringLiteral("开启混音"));
		return;
	}

	AUDIOHOOK::StartAudioRecord();
	LIVEROOM::EnableAux(true);

	ui.m_bAux->setEnabled(true);
	ui.m_bAux->setText(QStringLiteral("关闭混音"));
}
#endif

void ZegoSingleAnchorDialog::OnButtonAux()
{
	if (ui.m_bAux->text() == QStringLiteral("开启混音"))
	{
		ui.m_bAux->setText(QStringLiteral("开启中..."));
		ui.m_bAux->setEnabled(false);

#ifdef Q_OS_WIN

		hookDialog.searchMusicAppFromReg();
		if (hookDialog.exec() == QDialog::Rejected)
		{
			ui.m_bAux->setEnabled(true);
			ui.m_bAux->setText(QStringLiteral("开启混音"));
		}
		
#endif

#ifdef Q_OS_MAC

		BeginAux();
		ui.m_bAux->setEnabled(true);
		ui.m_bAux->setText(QStringLiteral("关闭混音"));
#endif

	}
	else
	{
		ui.m_bAux->setText(QStringLiteral("关闭中..."));
		ui.m_bAux->setEnabled(false);

#ifdef Q_OS_WIN
		if (isUseDefaultAux)
		{
			EndAux();
	
		}
		else
		{
			AUDIOHOOK::StopAudioRecord();
			LIVEROOM::EnableAux(false);
			AUDIOHOOK::UnInitAudioHook();
		
	    }
#else
		EndAux();
#endif
		ui.m_bAux->setEnabled(true);
		ui.m_bAux->setText(QStringLiteral("开启混音"));
	}
}

void ZegoSingleAnchorDialog::OnShowSnapShotImage(QImage *imageData)
{
	ZegoImageShowDialog imageShowDialog(imageData, imageData->width(), imageData->height(), m_pAVSettings);
	imageShowDialog.initDialog();
	imageShowDialog.exec();

	if (m_image)
	{
		delete[] m_image;
		m_image = nullptr;
	}

}

void ZegoSingleAnchorDialog::OnSwitchAudioDevice(int id)
{
	if (id < 0)
		return;

	if (id < m_vecAudioDeviceIDs.size())
	{
		LIVEROOM::SetAudioDevice(AV::AudioDevice_Input, m_vecAudioDeviceIDs[id].toStdString().c_str());
		m_pAVSettings->SetMicrophoneId(m_vecAudioDeviceIDs[id]);
		ui.m_cbMircoPhone->setCurrentIndex(id);
		update();
	}
}

void ZegoSingleAnchorDialog::OnSwitchVideoDevice(int id)
{
	if (id < 0)
		return;

	if (id < m_vecVideoDeviceIDs.size())
	{
		//若摄像头1需要选取的摄像头已被摄像头2选取，则交换摄像头
		if (id == ui.m_cbCamera2->currentIndex())
		{
			//先关闭所有正在使用的摄像头，避免交换时先后次序不同导致的抢占问题
			LIVEROOM::StopPreview();
			LIVEROOM::StopPreview(ZEGO::AV::PUBLISH_CHN_AUX);
			LIVEROOM::EnableCamera(false);
			LIVEROOM::EnableCamera(false, ZEGO::AV::PUBLISH_CHN_AUX);
			
			m_pAVSettings->SetCameraId2(m_pAVSettings->GetCameraId());

			int curLastCameraIndex = getCameraIndexFromID(m_pAVSettings->GetCameraId());
			if (curLastCameraIndex < 0)
				return;

			ui.m_cbCamera2->blockSignals(true);
			ui.m_cbCamera2->setCurrentIndex(curLastCameraIndex);
			ui.m_cbCamera2->blockSignals(false);
			
			LIVEROOM::SetVideoDevice(m_pAVSettings->GetCameraId().toStdString().c_str(), ZEGO::AV::PUBLISH_CHN_AUX);
		}

		LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[id].toStdString().c_str());
		
		m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[id]);
		ui.m_cbCamera->blockSignals(true);
		ui.m_cbCamera->setCurrentIndex(id);
		ui.m_cbCamera->blockSignals(false);
		update();
		
		LIVEROOM::EnableCamera(true);
		LIVEROOM::EnableCamera(true, ZEGO::AV::PUBLISH_CHN_AUX);
		LIVEROOM::StartPreview();
		LIVEROOM::StartPreview(ZEGO::AV::PUBLISH_CHN_AUX);

		qDebug() << "[SingleAnchorDialog::VideoDevice_Changed_1]: current video device_main : " << m_pAVSettings->GetCameraId() << " video device_aux : " << m_pAVSettings->GetCameraId2();
	}
}

void ZegoSingleAnchorDialog::OnSwitchVideoDevice2(int id)
{
	if (id < 0)
		return;

	if (id < m_vecVideoDeviceIDs.size())
	{
		//若摄像头1需要选取的摄像头已被摄像头2选取，则交换摄像头
		if (id == ui.m_cbCamera->currentIndex())
		{
			//先关闭所有正在使用的摄像头，避免交换时先后次序不同导致的抢占问题
			LIVEROOM::StopPreview();
			LIVEROOM::StopPreview(ZEGO::AV::PUBLISH_CHN_AUX);
			LIVEROOM::EnableCamera(false);
			LIVEROOM::EnableCamera(false, ZEGO::AV::PUBLISH_CHN_AUX);

			m_pAVSettings->SetCameraId(m_pAVSettings->GetCameraId2());

			int curLastCameraIndex = getCameraIndexFromID(m_pAVSettings->GetCameraId2());
			if (curLastCameraIndex < 0)
				return;

			ui.m_cbCamera->blockSignals(true);
			ui.m_cbCamera->setCurrentIndex(curLastCameraIndex);
			ui.m_cbCamera->blockSignals(false);
			LIVEROOM::SetVideoDevice(m_pAVSettings->GetCameraId2().toStdString().c_str());
		}
		
		LIVEROOM::SetVideoDevice(m_vecVideoDeviceIDs[id].toStdString().c_str(), ZEGO::AV::PUBLISH_CHN_AUX);
		m_pAVSettings->SetCameraId2(m_vecVideoDeviceIDs[id]);
		ui.m_cbCamera2->blockSignals(true);
		ui.m_cbCamera2->setCurrentIndex(id);
		ui.m_cbCamera2->blockSignals(false);
		update();

		LIVEROOM::EnableCamera(true);
		LIVEROOM::EnableCamera(true, ZEGO::AV::PUBLISH_CHN_AUX);
		LIVEROOM::StartPreview();
		LIVEROOM::StartPreview(ZEGO::AV::PUBLISH_CHN_AUX);

		qDebug() << "[SingleAnchorDialog::VideoDevice_Changed_2]: current video device_main : " << m_pAVSettings->GetCameraId() << " video device_aux : " << m_pAVSettings->GetCameraId2();
	}
}

void ZegoSingleAnchorDialog::mousePressEvent(QMouseEvent *event)
{
	mousePosition = event->pos();
	//只对标题栏范围内的鼠标事件进行处理

	if (mousePosition.y() <= pos_min_y)
		return;
	if (mousePosition.y() >= pos_max_y)
		return;
	isMousePressed = true;
}

void ZegoSingleAnchorDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (isMousePressed == true)
	{
		QPoint movePot = event->globalPos() - mousePosition;
		move(movePot);
	}
}

void ZegoSingleAnchorDialog::mouseReleaseEvent(QMouseEvent *event)
{
	isMousePressed = false;
}

void ZegoSingleAnchorDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	//双击标题栏同样可以放大缩小

	mousePosition = event->pos();
	//只对标题栏范围内的鼠标事件进行处理

	if (mousePosition.y() <= pos_min_y)
		return;
	if (mousePosition.y() >= pos_max_y)
		return;

	if (isMax)
	{
		this->showNormal();
		isMax = false;
	}
	else
	{
		this->showMaximized();
		isMax = true;
	}

}

void ZegoSingleAnchorDialog::closeEvent(QCloseEvent *e)
{
	QDialog::closeEvent(e);
	GetOut();
	//this->close();
	emit sigSaveVideoSettings(m_pAVSettings);
	m_lastDialog->show();
}

bool ZegoSingleAnchorDialog::eventFilter(QObject *target, QEvent *event)
{
	if (target == ui.m_edInput)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Return && (keyEvent->modifiers() & Qt::ControlModifier))
			{
				ui.m_edInput->insertPlainText("\n");
				return true;
			}
			else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				OnButtonSendMessage();
				return true;
			}
		}
	}
	else if (target == ui.m_zoneLiveView_Inner)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Escape && m_isLiveFullScreen)
			{
				//qDebug() << "clicl esc";
				//ui.m_zoneLiveView_Inner->setParent(ui.m_zoneLiveView);
				ui.m_zoneLiveView_Inner->setWindowFlags(Qt::SubWindow);
				ui.m_zoneLiveView_Inner->showNormal();
				ui.horizontalLayout_ForAVView->addWidget(ui.m_zoneLiveView_Inner);
				m_isLiveFullScreen = false;
				//取消直播窗口总在最顶层
				//ui.m_zoneLiveView_Inner->setWindowFlags(ui.m_zoneLiveView_Inner->windowFlags() &~Qt::WindowStaysOnTopHint);
				return true;
			}
			else if (keyEvent->key() == Qt::Key_Escape && !m_isLiveFullScreen)
			{
				return true;
			}
		}
	}
	
	return QDialog::eventFilter(target, event);
}
