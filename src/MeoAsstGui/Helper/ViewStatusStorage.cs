// MeoAssistanceGui - A part of the MeoAssistance-Arknight project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System;
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MeoAsstGui
{
    // 界面设置存储（读写json文件）
    public class ViewStatusStorage
    {
        private static string _configFilename = System.Environment.CurrentDirectory + "\\gui.json";
        private static JObject _viewStatus = new JObject();

        public static string Get(string key, string defalut_value)
        {
            if (_viewStatus.ContainsKey(key))
            {
                return _viewStatus[key].ToString();
            }
            else
            {
                return defalut_value;
            }
        }

        public static void Set(string key, string value)
        {
            _viewStatus[key] = value;
        }

        public static bool Load()
        {
            try
            {
                using (StreamReader sr = new StreamReader(_configFilename))
                {
                    string jsonStr = sr.ReadToEnd();
                    _viewStatus = (JObject)JsonConvert.DeserializeObject(jsonStr);
                    // 文件存在但为空，会读出来一个null，感觉c#这库有bug
                    if (_viewStatus == null)
                    {
                        _viewStatus = new JObject();
                    }
                }
            }
            catch (Exception)
            {
                _viewStatus = new JObject();
                return false;
            }
            return true;
        }

        public static bool Save()
        {
            try
            {
                using (StreamWriter sw = new StreamWriter(_configFilename))
                {
                    sw.Write(_viewStatus.ToString());
                }
            }
            catch (Exception)
            {
                return false;
            }
            return true;
        }
    }
}
