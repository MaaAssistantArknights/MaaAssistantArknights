// <copyright file="ViewStatusStorage.cs" company="MistEO">
// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MeoAsstGui
{
    // 界面设置存储（读写json文件）
    public class ViewStatusStorage
    {
        private static readonly string _configFilename = System.Environment.CurrentDirectory + "\\gui.json";
        private static readonly string _configBakFilename = System.Environment.CurrentDirectory + "\\gui.json.bak";
        private static JObject _viewStatus = new JObject();

        public static string Get(string key, string default_value)
        {
            if (_viewStatus.ContainsKey(key))
            {
                return _viewStatus[key].ToString();
            }
            else
            {
                return default_value;
            }
        }

        public static void Set(string key, string value)
        {
            _viewStatus[key] = value;
            Save();
        }

        public static bool Load(bool withRestore = true)
        {
            if (File.Exists(_configFilename))
            {
                try
                {
                    string jsonStr = File.ReadAllText(_configFilename);

                    if (jsonStr.Length <= 2 && File.Exists(_configBakFilename))
                    {
                        File.Copy(_configBakFilename, _configFilename, true);
                        jsonStr = File.ReadAllText(_configFilename);
                    }

                    // 文件存在但为空，会读出来一个null，感觉c#这库有bug，如果是null 就赋值一个空JObject
                    _viewStatus = (JObject)JsonConvert.DeserializeObject(jsonStr) ?? new JObject();
                }
                catch (Exception)
                {
                    if (withRestore && File.Exists(_configBakFilename))
                    {
                        File.Copy(_configBakFilename, _configFilename, true);
                        return Load(false);
                    }
                    else
                    {
                        _viewStatus = new JObject();
                        return false;
                    }
                }
            }
            else
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
                var jsonStr = _viewStatus.ToString();
                if (jsonStr.Length > 2)
                {
                    using (StreamWriter sw = new StreamWriter(_configFilename))
                    {
                        sw.Write(jsonStr);
                    }
                    if (new System.IO.FileInfo(_configFilename).Length > 2)
                    {
                        File.Copy(_configFilename, _configBakFilename, true);
                    }
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
