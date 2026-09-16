"""Test transient-toast deduplication and confirmed inventory deltas."""

from pathlib import Path
import subprocess

from test_material_craft_operators import extract

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/processing-byproduct-test"

FIXTURE = r"""
#include "Utils/ProcessingByproductAccumulator.h"
#include <meojson/json.hpp>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace asst;
using CraftOperation=MaterialCraftOperation;
enum class AsstMsg { SubTaskExtraInfo };
namespace cv {
struct Rect { int x=0,y=0,width=380,height=640; };
struct Mat {
    int id=0,cols=380,rows=640;
    bool empty()const{return id<0;}
    Mat operator()(const Rect&)const{return *this;}
    Mat clone()const{return *this;}
};
}
template<class T> T make_rect(const cv::Rect& rect){return rect;}
struct TaskConfig { std::vector<int> special_params={0,0,128};cv::Rect roi;int post_delay=0; };
struct Tasks { TaskConfig config;const TaskConfig* get(const char*)const{return &config;} } Task;
struct Matcher {
    cv::Mat image;Matcher(cv::Mat frame):image(frame){}
    void set_task_info(const char*){}void set_roi(const cv::Rect&){}
    std::optional<int> analyze()const{return image.id>0?std::optional(1):std::nullopt;}
};
struct Logger { template<class... T> void info(T&&...){} } Log;
struct Harness {
    MaterialInventory m_processing_byproducts;
    MaterialCraftRequest m_request;
    bool m_inventory_complete=false;
    json::value last;
    json::value basic_info_with_what(const std::string& what) {return json::object{{"what",what},{"details",json::object{}}};}
    void callback(AsstMsg,const json::value& event){last=event;}
    void callback_operation(const std::string&,const CraftOperation&,int);
    bool need_exit()const{return false;}
    bool craft_sleep(int)const{return false;}
    Harness* ctrler(){return this;}
    cv::Mat get_image(){throw std::runtime_error("Unexpected screenshot request");}
    MaterialInventory read_processing_byproducts(const cv::Mat& frame,int)const {
        return frame.id>0?MaterialInventory{{"30043",1}}:MaterialInventory{};
    }
    void capture_processing_byproducts(const cv::Mat&,int,const std::vector<cv::Mat>&);
};
// ACTUAL_CALLBACK
int main() {
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    { ProcessingByproductAccumulator a(1);require(a.confirmed().empty());
      a.observe({{"30051",1}});require(a.confirmed().empty());
      a.observe({{"30051",1}});require(a.confirmed()==MaterialInventory{{"30051",1}});
      for(int i=0;i<30;++i)a.observe({{"30051",1}});
      require(a.confirmed()==MaterialInventory{{"30051",1}}); }
    { ProcessingByproductAccumulator a(4);
      a.observe({{"30051",1}});a.observe({{"30051",1}});
      a.observe({{"30051",2}});require(a.confirmed().at("30051")==1);
      a.observe({{"30051",2}});require(a.confirmed().at("30051")==2);
      a.observe({{"30011",1}});a.observe({{"30011",1}});
      require(a.confirmed()==MaterialInventory{{"30051",2},{"30011",1}}); }
    { ProcessingByproductAccumulator a(2);
      a.observe({{"30051",1},{"30011",1}});a.observe({{"30051",1},{"30011",1}});
      require(a.confirmed().size()==2); }
    for(const auto& bad:std::vector<MaterialInventory>{
        {{"30051",0}},{{"30051",-1}},{{"30051",2}},{{"",1}},{{"30051",1},{"30011",1}}}) {
      ProcessingByproductAccumulator a(1);a.observe(bad);a.observe(bad);require(a.confirmed().empty()); }
    { ProcessingByproductAccumulator a(1);
      a.observe({{"30051",1}});a.observe({{"30051",1}});
      a.observe({{"30011",1}});a.observe({{"30011",1}});
      require(a.confirmed().empty()); }
    { ProcessingByproductAccumulator a(1);a.observe({});a.observe({});require(a.confirmed().empty()); }
    { ProcessingByproductAccumulator a(1);a.observe({{"30051",1}});a.observe({{"30011",1}});
      require(a.confirmed().empty()); }
    // A subsequent craft must not inherit a notification from the preceding craft.
    require(ProcessingByproductAccumulator(1).confirmed().empty());
    // Early toasts remain usable when the reward-page frame has no toast (or capture is interrupted).
    { Harness h;h.capture_processing_byproducts({-1},1,{{1},{1}});
      require(h.m_processing_byproducts==MaterialInventory{{"30043",1}});require(!h.m_inventory_complete); }
    { Harness h;h.capture_processing_byproducts({0},1,{{1},{1}});
      require(h.m_processing_byproducts==MaterialInventory{{"30043",1}}); }
    { Harness h;h.capture_processing_byproducts({1},1,{{1}});
      require(h.m_processing_byproducts==MaterialInventory{{"30043",1}}); }
    { Harness h;h.capture_processing_byproducts({-1},1,{{1}});require(h.m_processing_byproducts.empty()); }
    { Harness h;h.capture_processing_byproducts({1},1,{});require(h.m_processing_byproducts.empty()); }
    { Harness h;MaterialFormula f;f.item_id="30012";f.count=1;f.gold_cost=100;f.costs={{"30011",3}};
      h.m_request.inventory["4001"]=1000;h.m_processing_byproducts={{"old",1}};
      h.callback_operation("MaterialCraftOperationStarted",{f,1},0);require(h.m_processing_byproducts.empty());
      h.m_processing_byproducts={{"30011",1},{"30051",1}};
      h.callback_operation("MaterialCraftOperationCompleted",{f,1},0);
      const auto& detail=h.last.at("details");MaterialInventory deltas;
      for(const auto& row:detail.at("inventory_changes").as_array())deltas[row.at("item_id").as_string()]=row.at("count").as_integer();
      require(deltas==MaterialInventory{{"30012",1},{"30011",-2},{"30051",1},{"4001",-100}});
      require(detail.at("byproducts").as_array().size()==2);require(!detail.at("inventory_complete").as_boolean());
      h.callback_operation("MaterialCraftOperationStarted",{f,1},1);require(h.m_processing_byproducts.empty());
      h.callback_operation("MaterialCraftOperationCompleted",{f,1},1);require(h.last.at("details").at("byproducts").as_array().empty());
      // A skill book can also be the main output: combine both into a single inventory delta.
      f.item_id="3302";f.costs={{"3301",3}};h.m_processing_byproducts={{"3302",1}};
      h.callback_operation("MaterialCraftOperationCompleted",{f,1},2);
      deltas.clear();for(const auto& row:h.last.at("details").at("inventory_changes").as_array())deltas[row.at("item_id").as_string()]=row.at("count").as_integer();
      require(deltas.at("3302")==2&&deltas.at("3301")==-3);
      f.facility="Mfg";h.m_processing_byproducts={{"30051",1}};
      h.callback_operation("MaterialCraftOperationCompleted",{f,1},2);require(h.last.at("details").at("byproducts").as_array().empty()); }
    std::cout<<"Byproduct accumulator and callback: "<<checks<<" checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    callback = extract(
        (ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask.cpp").read_text(
            encoding="utf-8"
        ),
        "callback_operation",
    )
    callback += "\n" + extract(
        (
            ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask_Byproducts.cpp"
        ).read_text(encoding="utf-8"),
        "capture_processing_byproducts",
    )
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_CALLBACK", callback), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(ProcessingByproducts LANGUAGES CXX)\n"
        "add_executable(byproducts main.cpp)\n"
        f'target_include_directories(byproducts PRIVATE "{ROOT.as_posix()}/src/MaaCore" "{ROOT.as_posix()}/src/MaaUtils/include")\n'
        "target_compile_features(byproducts PRIVATE cxx_std_20)\n"
        "if(MSVC)\n target_compile_options(byproducts PRIVATE /utf-8)\nendif()\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    executable = next(
        path
        for path in (OUT / "native/Debug/byproducts.exe", OUT / "native/byproducts")
        if path.exists()
    )
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
