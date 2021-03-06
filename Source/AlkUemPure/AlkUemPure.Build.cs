// Copyright 2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
using UnrealBuildTool;

public class AlkUemPure : ModuleRules
{
  public AlkUemPure(ReadOnlyTargetRules Target) : base(Target)
  {
    bLegacyPublicIncludePaths = false;
      // ^ !!! fixes VC error: command line is too long to fit in debug record
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PrivateDependencyModuleNames.AddRange(new string[] {
      "Core",
      "CoreUObject",
      "Engine"
    });

    if (Target.Platform == UnrealTargetPlatform.Android)
    {
      // required customizations for the Android build process
      string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
      AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "AlkUemPure_APL.xml"));
    }
  }
}
