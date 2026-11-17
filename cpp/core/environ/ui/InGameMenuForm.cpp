#include "InGameMenuForm.h"
#include "cocos2d/MainScene.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UIText.h"
#include "MenuItemIntf.h"
#include "ui/UIHelper.h"
#include "tjsGlobalStringMap.h"
#include "csd/CsdUIFactory.h"
#include "2d/CCLayer.h"
#include <emscripten/html5.h>
#include <algorithm>
#include <cstdio>

using namespace cocos2d;
using namespace cocos2d::ui;

TVPInGameMenuForm *TVPInGameMenuForm::create(const std::string &title,
                                             tTJSNI_MenuItem *item) {
    auto *ret = new TVPInGameMenuForm;
    // 先建好根节点并自留引用，rearrangeLayout 直接对根节点做适配，
    // 不依赖 NodeMap 的 "background" 查找
    ret->_naviRoot = Csd::createNaviBar();
    ret->_bodyRoot = Csd::createListView();
    ret->initFromFile(ret->_naviRoot, ret->_bodyRoot, nullptr);
    // 不透明底色：子菜单逐级 push 时遮住下层表单，避免文字透出重叠
    ret->_bg = cocos2d::LayerColor::create(cocos2d::Color4B(16, 18, 26, 235));
    ret->addChild(ret->_bg, -1);
    ret->initMenu(title, item);
    ret->rearrangeLayout();
    return ret;
}

// 表单按 720x1080（标题栏 120 + 列表 960）的自然尺寸设计。
// 设计分辨率随窗口纵横比变化（宽屏时高度只有 ~460 单位），
// 若按宽度铺满会导致宽屏下文字过大、一屏放不下几行。
// 因此同时受宽、高约束取小：竖屏铺满宽度，宽屏收缩为居中面板。
// 窗口尺寸变化时由 TVPMainScene::onFrameSizeChanged 再次调用。
void TVPInGameMenuForm::rearrangeLayout() {
    cocos2d::Size sceneSize = TVPMainScene::GetInstance()->getGameNodeSize();
    setContentSize(sceneSize);

    const float scale = std::min(sceneSize.width / 720.0f,
                                 sceneSize.height / 1080.0f);
    const float formW = 720.0f * scale;
    const float formH = 1080.0f * scale;
    const float originX = (sceneSize.width - formW) / 2.0f;
    const float originY = sceneSize.height - formH;

    int canvasW = 0, canvasH = 0;
    emscripten_get_canvas_element_size("#canvas", &canvasW, &canvasH);

    if(_naviRoot) {
        _naviRoot->setAnchorPoint(cocos2d::Vec2::ZERO);
        _naviRoot->setScale(scale);
        _naviRoot->setPosition(originX, originY + 960.0f * scale);
    }
    if(_bodyRoot) {
        _bodyRoot->setAnchorPoint(cocos2d::Vec2::ZERO);
        _bodyRoot->setScale(scale);
        _bodyRoot->setPosition(originX, originY);
    }
    if(_bg) {
        // 底色铺满整个场景，菜单面板之外的游戏画面也一并压暗
        _bg->setContentSize(sceneSize);
        _bg->setPosition(0, 0);
    }
}

void TVPInGameMenuForm::bindHeaderController(const NodeMap &allNodes) {
    _title = allNodes.findController<Button>("title");
    if(_title)
        _title->setEnabled(false);
}

void TVPInGameMenuForm::bindBodyController(const NodeMap &allNodes) {
    _list = allNodes.findController<ListView>("list");
    if(NaviBar.Left) {
        NaviBar.Left->addClickEventListener([this](cocos2d::Ref *) {
            TVPMainScene::GetInstance()->popUIForm(this);
        });
    }
}

void TVPInGameMenuForm::initMenu(const std::string &title,
                                 tTJSNI_MenuItem *item) {
    _list->removeAllItems();
    if(_title) {
        if(title.empty()) {
            ttstr caption;
            item->GetCaption(caption);
            _title->setTitleText(caption.AsStdString());
        } else {
            _title->setTitleText(title);
        }
    }

    int count = item->GetChildren().size();
    int idx = 0;
    ttstr seperator = TJS::TJSMapGlobalStringMap(TJS_W("-"));
    for(int i = 0; i < count; ++i) {
        tTJSNI_MenuItem *subitem =
            static_cast<tTJSNI_MenuItem *>(item->GetChildren().at(i));
        ttstr caption;
        subitem->GetCaption(caption);
        if(caption.IsEmpty() || caption == TJS_W("+"))
            continue;
        _list->pushBackCustomItem(
            createMenuItem(idx, subitem, caption.AsStdString()));
        if(caption != seperator)
            ++idx;
    }
}

cocos2d::ui::Widget *
TVPInGameMenuForm::createMenuItem(int idx, tTJSNI_MenuItem *item,
                                  const std::string &caption) {
    iPreferenceItem *ret = nullptr;
    const cocos2d::Size &size = _list->getContentSize();
    if(!item->GetChildren().empty()) {
        ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            TVPMainScene::GetInstance()->pushUIForm(create(caption, item));
        });
    } else if(item->GetGroup() > 0 || item->GetRadio()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) {
            item->OnClick();
            TVPMainScene::GetInstance()->popAllUIForm();
        };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(item->GetChecked()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) { item->OnClick(); };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(caption == "-") {
        float w = size.width;
        Widget *sep =
            Csd::createSeperateItem(w, 2.0f, Color4F(0.6f, 0.6f, 0.6f, 1.0f));
        return sep;
    } else {
        ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            TVPMainScene::GetInstance()->scheduleOnce(
                [c = TVPMainScene::GetInstance()](float) { c->popAllUIForm(); },
                0, "close_menu");
            item->OnClick();
        });
        ret->setTouchEnabled(true);
    }
    return ret;
}

void TVPShowPopMenu(tTJSNI_MenuItem *menu) {
    TVPMainScene::GetInstance()->pushUIForm(
        TVPInGameMenuForm::create(std::string(), menu));
}
