#include "GraphicsAPI.h"

// RHI は実質ヘッダ中心のプロジェクトだが、静的ライブラリとして .lib を出力するために
// 最低 1 つの translation unit を残している。App / Graphics / Backends はリンク時に
// Xelqoria.RHI.lib を参照するため、このファイルを削除すると .lib が生成されなくなる。

namespace Xelqoria::RHI
{
}
