#pragma once
#include "PreferenceForm.h"

class tTJSNI_MenuItem;

class TVPInGameMenuForm : public iTVPBaseForm {
public:
    static TVPInGameMenuForm *create(const std::string &title,
                                     tTJSNI_MenuItem *item);

    void bindHeaderController(const NodeMap &allNodes) override;
    void bindBodyController(const NodeMap &allNodes) override;
    void bindFooterController(const NodeMap &allNodes) override {}

    // 代码生成的 UI 按 720 逻辑宽布局，需按场景尺寸适配，
    // 否则标题栏落在屏幕左下角、列表错位（cocos 原点在左下角）
    void rearrangeLayout() override;

    void initMenu(const std::string &title, tTJSNI_MenuItem *item);

private:
    cocos2d::ui::Widget *createMenuItem(int idx, tTJSNI_MenuItem *item,
                                        const std::string &caption);

    cocos2d::ui::ListView *_list{};
    cocos2d::ui::Button *_title{};
    cocos2d::Node *_naviRoot{};   // createNaviBar 返回的标题栏根节点
    cocos2d::Node *_bodyRoot{};   // createListView 返回的列表根节点
    cocos2d::LayerColor *_bg{};   // 不透明底色，遮住下层表单
};
