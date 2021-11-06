/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <memory>

#include "AsstMsg.h"

namespace cv {
    class Mat;
}

namespace asst {
    class AbstractTask {
    public:
        AbstractTask(AsstCallback callback, void* callback_arg);
        virtual ~AbstractTask() = default;
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) = default;

        virtual bool run() = 0;

        virtual void set_exit_flag(bool* exit_flag);
        virtual void set_retry_times(int times) { m_retry_times = times; }
        virtual int get_retry_times() { return m_retry_times; }
        virtual void set_task_chain(std::string name) { m_task_chain = std::move(name); }
        virtual const std::string& get_task_chain() { return m_task_chain; }
        virtual void on_run_fails(int retry_times) { ; }

    protected:
        virtual bool sleep(unsigned millisecond);
        virtual bool save_image(const cv::Mat& iamge, const std::string& dir);
        virtual bool need_exit() const noexcept;

        virtual void click_return_button();

        AsstCallback m_callback;
        void* m_callback_arg = NULL;
        bool* m_exit_flag = NULL;
        std::string m_task_chain;
        int m_retry_times = INT_MAX;
    };
}