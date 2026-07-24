#ifndef SOUL_UI_ICON_H
#define SOUL_UI_ICON_H

#include <QIcon>
#include <QString>
#include "soul/ui/design_constants.h"

namespace sc {

/**
 * @class Icon
 * @brief 图标管理工具类
 *
 * Icon 提供多种方式创建图标：
 * - fromResource()：从资源文件加载
 * - fromFont()：从字体图标加载
 * - fromColor()：从颜色创建简单图标
 * - appIcon()：获取应用图标
 *
 * 使用方式：
 * @code
 * QIcon icon = Icon::fromResource(":/icons/app.png");
 * QIcon icon = Icon::fromFont("MaterialIcons", "\uE87C");
 * @endcode
 */
class Icon {
public:
    /**
     * @brief 从资源文件加载图标
     * @param path 资源路径
     * @return QIcon 对象
     */
    static QIcon fromResource(const QString& path);

    /**
     * @brief 从字体图标加载
     * @param fontName 字体名称
     * @param character 字符编码
     * @return QIcon 对象
     */
    static QIcon fromFont(const QString& fontName, QString character);

    /**
     * @brief 从颜色创建图标
     * @param color 颜色
     * @param size 图标大小（像素）
     * @return QIcon 对象
     */
    static QIcon fromColor(const QColor& color, int size = design::DEFAULT_ICON_SIZE);

    /**
     * @brief 获取应用图标
     * @return 应用图标
     */
    static QIcon appIcon();

    /**
     * @brief 设置应用图标
     * @param icon 图标
     */
    static void setAppIcon(const QIcon& icon);

private:
    static QIcon s_appIcon;
};

}

#endif
