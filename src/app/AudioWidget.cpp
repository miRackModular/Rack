#include "app.hpp"
#include "audio.hpp"
#include "engine.hpp"

namespace rack {


struct AudioDriverItem : ChoiceMenuItem {
	AudioIO *audioIO;
	int driver;
	void onAction(EventAction &e) override {
		audioIO->setDriver(driver);
	}
};

struct AudioDriverChoice : LedDisplayChoice {
	AudioWidget *audioWidget;
	void onAction(EventAction &e) override {
		Menu *menu = gScene->createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Audio driver"));
		for (int driver : audioWidget->audioIO->getDrivers()) {
			AudioDriverItem *item = ChoiceMenuItem::create<AudioDriverItem>(this,
				audioWidget->audioIO->getDriverName(driver),
				CHECKMARK(driver == audioWidget->audioIO->driver)
			);
			item->audioIO = audioWidget->audioIO;
			item->driver = driver;
			menu->addChild(item);
		}
		gScene->adjustMenuPosition(menu);		
	}
	void onChange(EventChange &e) override {
		text = audioWidget->audioIO->getDriverName(audioWidget->audioIO->driver);
	}
};


struct AudioDeviceItem : ChoiceMenuItem {
	AudioIO *audioIO;
	int device;
	int offset;
	void onAction(EventAction &e) override {
		audioIO->setDevice(device, offset);
	}
};

struct AudioDeviceChoice : LedDisplayChoice {
	AudioWidget *audioWidget;
	/** Prevents devices with a ridiculous number of channels from being displayed */
	int maxTotalChannels = 128;

	void onAction(EventAction &e) override {
		Menu *menu = gScene->createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Audio device"));
		int deviceCount = audioWidget->audioIO->getDeviceCount();
		{
			AudioDeviceItem *item = ChoiceMenuItem::create<AudioDeviceItem>(this,
				"(No device)",
				CHECKMARK(-1 == audioWidget->audioIO->device)
			);
			item->audioIO = audioWidget->audioIO;
			item->device = -1;
			menu->addChild(item);
		}
		for (int device = 0; device < deviceCount; device++) {
			int channels = min(maxTotalChannels, audioWidget->audioIO->getDeviceChannels(device));
			for (int offset = 0; offset < channels; offset += audioWidget->audioIO->maxChannels) {
				AudioDeviceItem *item = ChoiceMenuItem::create<AudioDeviceItem>(this,
					audioWidget->audioIO->getDeviceDetail(device, offset),
					CHECKMARK(device == audioWidget->audioIO->device && offset == audioWidget->audioIO->offset)
				);
				item->audioIO = audioWidget->audioIO;
				item->device = device;
				item->offset = offset;
				menu->addChild(item);
			}
		}
		gScene->adjustMenuPosition(menu);		
	}
	void onChange(EventChange &e) override {
		text = audioWidget->audioIO->getDeviceDetail(audioWidget->audioIO->device, audioWidget->audioIO->offset);
		if (text.empty()) {
			text = "(No device)";
			color.a = 0.5f;
		}
		else {
			color.a = 1.f;
		}
	}
};


struct AudioSampleRateItem : ChoiceMenuItem {
	AudioIO *audioIO;
	int sampleRate;
	void onAction(EventAction &e) override {
		audioIO->setSampleRate(sampleRate);
		engineSetSampleRate(sampleRate);
	}
};

struct AudioSampleRateChoice : LedDisplayChoice {
	AudioWidget *audioWidget;
	void onAction(EventAction &e) override {
		Menu *menu = gScene->createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Sample rate"));
		std::vector<int> sampleRates = audioWidget->audioIO->getSampleRates();
		if (sampleRates.empty()) {
			menu->addChild(construct<MenuLabel>(&MenuLabel::text, "(Locked by device)"));
		}
		for (int sampleRate : sampleRates) {
			AudioSampleRateItem *item = ChoiceMenuItem::create<AudioSampleRateItem>(this,
				stringf("%d Hz", sampleRate),
				CHECKMARK(sampleRate == audioWidget->audioIO->sampleRate)
			);
			item->audioIO = audioWidget->audioIO;
			item->sampleRate = sampleRate;
			menu->addChild(item);
		}
		gScene->adjustMenuPosition(menu);		
	}
	void onChange(EventChange &e) override {
		text = stringf("%g kHz", audioWidget->audioIO->sampleRate / 1000.f);
	}
};


struct AudioBlockSizeItem : ChoiceMenuItem {
	AudioIO *audioIO;
	int blockSize;
	void onAction(EventAction &e) override {
		audioIO->setBlockSize(blockSize);
	}
};

struct AudioBlockSizeChoice : LedDisplayChoice {
	AudioWidget *audioWidget;
	void onAction(EventAction &e) override {
		Menu *menu = gScene->createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Block size"));
		std::vector<int> blockSizes = audioWidget->audioIO->getBlockSizes();
		if (blockSizes.empty()) {
			menu->addChild(construct<MenuLabel>(&MenuLabel::text, "(Locked by device)"));
		}
		for (int blockSize : blockSizes) {
			float latency = (float) blockSize / audioWidget->audioIO->sampleRate * 1000.0;
			AudioBlockSizeItem *item = ChoiceMenuItem::create<AudioBlockSizeItem>(this,
				stringf("%d (%.1f ms)", blockSize, latency),
				CHECKMARK(blockSize == audioWidget->audioIO->blockSize)
			);
			item->audioIO = audioWidget->audioIO;
			item->blockSize = blockSize;
			menu->addChild(item);
		}
		gScene->adjustMenuPosition(menu);		
	}
	void onChange(EventChange &e) override {
		text = stringf("%d", audioWidget->audioIO->blockSize);
	}
};


AudioWidget::AudioWidget() {
	canSquash = true;

	Vec pos = Vec();

	AudioDriverChoice *driverChoice = Widget::create<AudioDriverChoice>(pos);
	driverChoice->audioWidget = this;
	addChild(driverChoice);
	pos = driverChoice->box.getBottomLeft();
	this->driverChoice = driverChoice;

	this->driverSeparator = Widget::create<LedDisplaySeparator>(pos);
	addChild(this->driverSeparator);

	AudioDeviceChoice *deviceChoice = Widget::create<AudioDeviceChoice>(pos);
	deviceChoice->audioWidget = this;
	addChild(deviceChoice);
	pos = deviceChoice->box.getBottomLeft();
	this->deviceChoice = deviceChoice;

	this->deviceSeparator = Widget::create<LedDisplaySeparator>(pos);
	addChild(this->deviceSeparator);

	AudioSampleRateChoice *sampleRateChoice = Widget::create<AudioSampleRateChoice>(pos);
	sampleRateChoice->audioWidget = this;
	addChild(sampleRateChoice);
	this->sampleRateChoice = sampleRateChoice;

	this->sampleRateSeparator = Widget::create<LedDisplaySeparator>(pos);
	this->sampleRateSeparator->box.size.y = this->sampleRateChoice->box.size.y;
	addChild(this->sampleRateSeparator);

	AudioBlockSizeChoice *bufferSizeChoice = Widget::create<AudioBlockSizeChoice>(pos);
	bufferSizeChoice->audioWidget = this;
	addChild(bufferSizeChoice);
	this->bufferSizeChoice = bufferSizeChoice;

	box.size = mm2px(Vec(44, 28));	
}

void AudioWidget::onChange(EventChange &e) {
	driverChoice->onChange(e);
	deviceChoice->onChange(e);	
	sampleRateChoice->onChange(e);
	bufferSizeChoice->onChange(e);
}

void AudioWidget::onResize() {
	this->driverChoice->box.size.x = box.size.x;
	this->driverSeparator->box.size.x = box.size.x;
	this->deviceChoice->box.size.x = box.size.x;
	this->deviceSeparator->box.size.x = box.size.x;
	this->sampleRateChoice->box.size.x = box.size.x / 2;
	this->sampleRateSeparator->box.pos.x = box.size.x / 2;
	this->bufferSizeChoice->box.pos.x = box.size.x / 2;
	this->bufferSizeChoice->box.size.x = box.size.x / 2;

	LedDisplay::onResize();
}


} // namespace rack
