#include "soul/utils/clipboard/clipboard_utils.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <memory>

namespace sc {

bool ClipboardUtils::setText(const QString& text) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
    return true;
}

QString ClipboardUtils::getText() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    return clipboard->text();
}

bool ClipboardUtils::setHtml(const QString& html) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    auto mimeData = std::unique_ptr<QMimeData>(new QMimeData());
    mimeData->setHtml(html);
    clipboard->setMimeData(mimeData.release());
    return true;
}

QString ClipboardUtils::getHtml() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (mimeData && mimeData->hasHtml()) {
        return mimeData->html();
    }
    return QString();
}

bool ClipboardUtils::setImage(const QImage& image) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
    return true;
}

QImage ClipboardUtils::getImage() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    return clipboard->image();
}

bool ClipboardUtils::setUrls(const QList<QUrl>& urls) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    auto mimeData = std::unique_ptr<QMimeData>(new QMimeData());
    mimeData->setUrls(urls);
    clipboard->setMimeData(mimeData.release());
    return true;
}

QList<QUrl> ClipboardUtils::getUrls() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        return mimeData->urls();
    }
    return QList<QUrl>();
}

bool ClipboardUtils::hasText() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasText();
}

bool ClipboardUtils::hasImage() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasImage();
}

bool ClipboardUtils::hasUrls() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasUrls();
}

void ClipboardUtils::clear() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->clear();
}

}