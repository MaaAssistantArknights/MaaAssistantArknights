// <copyright file="PropertyDependsOnAttribute.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System;

namespace MaaWpfGui.Utilities;

/// <summary>
/// 标记一个属性依赖于其他属性，当被依赖的属性改变时，自动通知当前属性。
/// <para>同实例：<c>[PropertyDependsOn(nameof(SomeProp))]</c></para>
/// <para>跨实例：<c>[PropertyDependsOn(typeof(OwnerType), nameof(OwnerType.SomeProp))]</c></para>
/// <para>全局刷新（语言切换等）：<c>[PropertyDependsOn]</c></para>
/// </summary>
[AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false)]
public class PropertyDependsOnAttribute : Attribute
{
    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyDependsOnAttribute"/> class.
    /// 无参：仅在全局刷新（如语言切换）时通知。
    /// </summary>
    public PropertyDependsOnAttribute()
    {
        PropertyNames = [];
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyDependsOnAttribute"/> class.
    /// 依赖同实例的指定属性。
    /// </summary>
    /// <param name="propertyName">属性名，支持 nameof()。</param>
    public PropertyDependsOnAttribute(string propertyName)
    {
        PropertyNames = [propertyName];
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyDependsOnAttribute"/> class.
    /// 依赖同实例的多个属性。
    /// </summary>
    /// <param name="propertyName">属性名，支持 nameof()。</param>
    /// <param name="propertyNames">其他属性名，支持 nameof()。</param>
    public PropertyDependsOnAttribute(string propertyName, params string[] propertyNames)
    {
        PropertyNames = [propertyName, .. propertyNames];
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyDependsOnAttribute"/> class.
    /// 跨实例依赖：依赖另一个类型实例上的属性。
    /// 用法：<c>[PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]</c>
    /// </summary>
    /// <param name="ownerType">拥有该属性的类型，框架会通过其静态 Instance 属性获取实例。</param>
    /// <param name="propertyName">属性名，支持 nameof()。</param>
    public PropertyDependsOnAttribute(Type ownerType, string propertyName)
    {
        OwnerType = ownerType;
        PropertyNames = [propertyName];
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyDependsOnAttribute"/> class.
    /// 跨实例依赖：依赖另一个类型实例上的多个属性。
    /// </summary>
    /// <param name="ownerType">拥有该属性的类型，框架会通过其静态 Instance 属性获取实例。</param>
    /// <param name="propertyName">属性名，支持 nameof()。</param>
    /// <param name="propertyNames">其他属性名，支持 nameof()。</param>
    public PropertyDependsOnAttribute(Type ownerType, string propertyName, params string[] propertyNames)
    {
        OwnerType = ownerType;
        PropertyNames = [propertyName, .. propertyNames];
    }

    /// <summary>
    /// Gets 被依赖的属性名列表。
    /// </summary>
    public string[] PropertyNames { get; init; }

    /// <summary>
    /// Gets 跨实例依赖时，拥有该属性的类型。为 null 时表示同实例依赖。
    /// </summary>
    public Type? OwnerType { get; init; }
}
