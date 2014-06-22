/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;  

import android.app.Activity;  
import android.os.Bundle;  

import android.view.View;
import android.view.Window;

import android.webkit.WebView;
import android.webkit.WebSettings;
import android.webkit.WebSettings.LayoutAlgorithm;  
import android.webkit.WebSettings.RenderPriority;
import android.webkit.WebViewClient;

import android.content.Context;

public class FastWikiAbout extends Activity {  
	WebView my_view;  
	WebSettings websettings;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		requestWindowFeature(Window.FEATURE_NO_TITLE);  
		setContentView(R.layout.about);  

		my_view = (WebView)findViewById(R.id.about_wv);  

		websettings = my_view.getSettings();  

		websettings.setAppCacheMaxSize(0);
		websettings.setAppCacheEnabled(false);
		websettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
		websettings.setSupportZoom(false);

		my_view.loadDataWithBaseURL("", WikiAbout(), "text/html", "utf8", "");
	}

	public native String WikiAbout();
}
