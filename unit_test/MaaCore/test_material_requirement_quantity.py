"""Exercise the actual quantity parser with controlled OCR outputs, without an emulator."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/requirement-check/parser"

FIXTURE = r"""
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
struct Rect { int x,y,width,height; };
namespace cv {
struct Mat { int cols=1280,rows=720; bool empty() const { return false; } };
struct Size {}; constexpr int INTER_CUBIC=0,THRESH_BINARY=0,COLOR_GRAY2BGR=0;
void resize(const Mat&,Mat&,Size,int,int,int) {}
void extractChannel(const Mat&,Mat&,int) {}
void threshold(const Mat&,Mat&,int,int,int) {}
void cvtColor(const Mat&,Mat&,int) {}
}
cv::Mat make_roi(const cv::Mat& image,Rect) { return image; }
struct OcrTaskInfo { Rect roi{}; bool use_raw=false; };
struct Registry { template<class T=OcrTaskInfo> auto get(const std::string&) { return std::make_shared<T>(); } } Task;
struct Logger { template<class... T> void info(T&&...){} } Log;
struct Result { std::string text; double score; };
std::vector<std::optional<Result>> responses;
int calls=0; bool stopped=false,cancel_after_first=false;
struct RegionOCRer {
    RegionOCRer(const cv::Mat&){}
    void set_task_info(std::shared_ptr<OcrTaskInfo>){}
    std::optional<Result> analyze(){
        auto result=calls<responses.size()?responses[calls]:std::nullopt;
        ++calls;
        if(cancel_after_first)stopped=true;
        return result;
    }
};
struct Harness {
    cv::Mat m_image;
    std::function<bool()> m_cancel_check=[] { return stopped; };
    bool parse_quantity(const std::string&,int&,int&)const;
};
// ACTUAL_METHOD
int main(){
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    auto reset=[] {calls=0;stopped=false;cancel_after_first=false;};
    auto run=[&](bool expected){int owned=-1,required=-1;bool ok=Harness{}.parse_quantity("quantity",owned,required);require(ok==expected);if(!ok)require(owned==-1&&required==-1);};
    for(const std::string text:{"0/4","3/4","13/4","4/4","2147483647/1"}){
        reset();responses={Result{text,0.99},Result{text,0.99}};
        int owned=-1,required=-1;require(Harness{}.parse_quantity("quantity",owned,required));
        auto slash=text.find('/');require(owned==std::stoi(text.substr(0,slash))&&required==std::stoi(text.substr(slash+1)));
        require(calls==2);
    }
    for(const std::string text:{"","374","3 4","3//4","3/4/5","-1/4","3/0","3/-4","x3/4","3/4x","O/4","3/I","999999999999999999/4","3/999999999999999999"}){
        reset();responses={Result{text,0.99},Result{text,0.99}};run(false);
    }
    for(double score:{0.949,std::nan(""),static_cast<double>(INFINITY)}){
        reset();responses={Result{"3/4",score},Result{"3/4",0.99}};run(false);
        reset();responses={Result{"3/4",0.99},Result{"3/4",score}};run(false);
    }
    reset();responses={Result{"3/4",0.95},Result{"3/4",0.95}};run(true);
    reset();responses={Result{"3/4",0.99},Result{"8/4",0.99}};run(false);
    reset();responses={Result{"0/4",0.944},Result{"0/4",0.94},Result{"0/4",0.99},Result{"0/4",0.99}};run(true);require(calls==4);
    reset();responses={Result{"3/4",0.79},Result{"3/4",0.85},Result{"8/4",0.99},Result{"8/4",0.98}};
    {int owned=-1,required=-1;require(Harness{}.parse_quantity("quantity",owned,required));require(owned==8&&required==4);}
    reset();responses={Result{"0/4",0.94},Result{"0/4",0.94},Result{"0/4",0.99},Result{"8/4",0.99}};run(false);
    reset();responses={Result{"0/4",0.94},Result{"0/4",0.94},Result{"0/4",0.99},Result{"0/4",0.94}};run(false);
    reset();responses={std::nullopt};run(false);
    reset();responses={Result{"3/4",0.99},std::nullopt};run(false);
    reset();stopped=true;run(false);require(calls==0);
    reset();cancel_after_first=true;responses={Result{"3/4",0.99}};run(false);require(calls==1);
    std::cout<<"Quantity parser: "<<checks<<" checks passed\n";
}
"""


def main():
    source = (
        ROOT / "src/MaaCore/Vision/Miscellaneous/MaterialRequirementImageAnalyzer.cpp"
    ).read_text(encoding="utf-8")
    start = source.index("bool MaterialRequirementImageAnalyzer::parse_quantity(")
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    method = source[start:end].replace(
        "MaterialRequirementImageAnalyzer::", "Harness::"
    )
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_METHOD", method), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(QuantityParser LANGUAGES CXX)\n"
        "add_executable(parser main.cpp)\ntarget_compile_features(parser PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    subprocess.run(
        [
            str(
                next(
                    p
                    for p in (OUT / "native/Debug/parser.exe", OUT / "native/parser")
                    if p.exists()
                )
            )
        ],
        check=True,
    )


if __name__ == "__main__":
    main()
