#include "SupportListAnalyzer.h"

#include <algorithm>
#include <regex>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"
#include "Vision/TemplDetOCRer.h"

bool asst::SupportListAnalyzer::analyze()
{
    LogTraceFunction;

    TemplDetOCRer analyzer(m_image);

    // 暂且写不动了，但是思路都想好了，大概是抄一下 TemplDetOCRer 的逻辑
    // 1. 根据 SupportListAnalyzer-Friend 和 SupportListAnalyzer-NonFriend" 找位置
    // 2. 根据位置位移后记录干员名称、精英化阶段和等级，精英化阶段需要先 inRange([250, 250, 250], [255, 255, 255]) 后再进行模版匹配
    // 3. 当页数不为 MAX 的时候记录一下 rect.x 最大的那个干员的截图，有用，有用

    return false;
}
