/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.preference.*;

import com.actionbarsherlock.app.SherlockPreferenceActivity;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class SettingFrame extends SherlockPreferenceActivity {
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.settings); 

		PreferenceCategory inlinePrefCat = new PreferenceCategory(this);
		inlinePrefCat.setTitle("hello");
		getPreferenceScreen().addPreference(inlinePrefCat);

		CheckBoxPreference checkboxPref = new CheckBoxPreference(this);
		checkboxPref.setKey("checkbox_preference");
		checkboxPref.setTitle("add box2");
		checkboxPref.setSummary("add box3");

		inlinePrefCat.addPreference(checkboxPref);

	}
}

