/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;  

import android.os.Bundle;  

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.app.ActionBar;

import android.view.View;
import android.view.Window;
import android.widget.TextView;

import android.webkit.WebView;
import android.webkit.WebSettings;
import android.webkit.WebSettings.LayoutAlgorithm;  
import android.webkit.WebSettings.RenderPriority;
import android.webkit.WebViewClient;

import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;

import android.content.Context;

public class FastWikiAbout extends SherlockActivity {  
	WebView my_view;  
	WebSettings websettings;
	private TextView m_text = null;
	private ViewGroup customViewForDictView;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.about);  

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.about_bar, null);
		ab.setCustomView(customViewForDictView);

		m_text = (TextView)customViewForDictView.findViewById(R.id.about_text);
		m_text.setText(N("FW_ABOUT_TITLE"));

		my_view = (WebView)findViewById(R.id.about_wv);  

		websettings = my_view.getSettings();  

		websettings.setAppCacheMaxSize(0);
		websettings.setAppCacheEnabled(false);
		websettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
		websettings.setSupportZoom(false);

		my_view.loadDataWithBaseURL("", WikiAbout(), "text/html", "utf8", "");

		init_full_screen();
	}

	private void init_full_screen()
	{
		if (WikiGetFullScreenFlag() == 1) {
			set_full_screen();
		}
	}

    private void set_full_screen()
	{
         this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				 	WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }

	public native String WikiAbout();
	public native String N(String name);
	public native int WikiGetFullScreenFlag();
}
