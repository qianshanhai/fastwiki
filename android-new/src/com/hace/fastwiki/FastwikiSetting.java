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

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.app.ActionBar;

import android.widget.RadioButton;  
import android.widget.RadioGroup;
import android.widget.ScrollView;

import android.widget.TextView;
import android.widget.Toast;
import android.view.View;
import android.view.LayoutInflater;
import android.widget.ImageButton;
import android.widget.Button;
import android.widget.AdapterView;  
import android.widget.AdapterView.OnItemSelectedListener;  
import android.widget.ArrayAdapter;  
import android.widget.Spinner;

import android.widget.CheckBox;  
import android.widget.CompoundButton; 
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.view.WindowManager;

import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.text.Html;
import android.text.Spanned;

public class FastwikiSetting extends SherlockActivity {
	TextView m_tmp_text = null;
	ScrollView setting_scroll = null;

	TextView show_text = null;
	TextView show_line = null;
	ImageButton show_back = null;
	ImageButton show_forward = null;

	CheckBox m_translate_box = null;
	CheckBox m_full_screen_box = null;
	CheckBox m_body_image_box = null;

	String [] m_lang_list = null;
	private ViewGroup customViewForDictView;
	TextView m_head_text = null;
	TextView m_body_image_path = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.fw_setting);  

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.setting_bar, null);
		ab.setCustomView(customViewForDictView);

		m_head_text = (TextView)customViewForDictView.findViewById(R.id.setting_text);
		m_head_text.setText(N("FW_SETTING"));

		setting_scroll = (ScrollView)findViewById(R.id.setting_scroll);

		set_one_text(R.id.language_text, "FW_SEL_LANG");
		set_one_text(R.id.home_page_text, "FW_HOME_PAGE");
		set_one_text(R.id.mutil_dict_text, "FW_MUTIL_DICT_SET");
		set_one_text(R.id.translate_need_text, "FW_NEED_TRANS");
		set_one_text(R.id.translate_show_text, "FW_TRANS_SHOW_LINES");
		set_one_text(R.id.translate_default_text, "FW_DEFAULT_TRANS_DICT");
		set_one_text(R.id.full_screen, "FW_FULL_SCREEN");
		set_one_text(R.id.random_text, "FW_RANDOM_ARTICLE");

		String [] m = {
			N("FW_SEL_ENGLISH"),
			N("FW_SEL_CHINESE")
		};
		Spinner language = new_spinner(R.id.language_sel, m, GetUseLanguage());
		language.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetUseLanguage(idx);
				show_short_msg(N("FW_SEL_LANG_MSG"));
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		});  

		String [] home_page_desc = {
			N("FW_HP_BLANK"),
			N("FW_HP_LAST_READ"),
			N("FW_HP_CURR_DATE")
		};
		Spinner home_page = new_spinner(R.id.home_page_list, home_page_desc, GetHomePageFlag()); 
		home_page.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetHomePageFlag(idx);
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		}); 

		String [] random_desc = {
			N("FW_RANDOM_HISTORY"),
			N("FW_RANDOM_FAVORITE"),
			N("FW_RANDOM_SELECTED_LANG"),
			N("FW_RANDOM_ALL_LANG")
		};
		Spinner random = new_spinner(R.id.random_list, random_desc, GetRandomFlag()); 
		random.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetRandomFlag(idx);
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		}); 

		m_full_screen_box = (CheckBox)findViewById(R.id.full_screen_box);
		if (WikiGetFullScreenFlag() == 0) {
			m_full_screen_box.setChecked(false);
		} else {
			m_full_screen_box.setChecked(true);
		}

		m_full_screen_box.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener(){ 
			@Override 
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) { 
				switch_full_screen();
			}
		});

		String [] mutil_dict_desc = {
			N("FW_SORT_BY_TITLE"),
			N("FW_SORT_BY_DICT")
		};
		Spinner mutil_dict = new_spinner(R.id.mutil_dict_list, mutil_dict_desc, GetMutilLangListMode());
		mutil_dict.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetMutilLangListMode(idx);
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		});

		m_lang_list = WikiLangList();
		String curr_default = TranslateDefault();

		int default_pos = 0;

		for (int i = 0; i < m_lang_list.length; i++) {
			if (m_lang_list[i].equals(curr_default)) {
				default_pos = i;
				break;
			}
		}

		if (m_lang_list.length == 0) {
			m_lang_list = new String[1];
			m_lang_list[0] = "No Dictionary";
		}

		Spinner trans_default = new_spinner(R.id.translate_default_list, m_lang_list, default_pos);
		trans_default.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetTranslateDefault(m_lang_list[idx]);
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		});

		m_translate_box = (CheckBox)findViewById(R.id.translate_box);
		if (GetNeedTranslate() == 0) {
			m_translate_box.setChecked(false);
		} else {
			m_translate_box.setChecked(true);
		}

		m_translate_box.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener(){ 
			@Override 
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) { 
				if (isChecked) {
					SetNeedTranslate(1);
					show_short_msg(N("FW_TRANS_ALERT_MSG"));
				} else {
					SetNeedTranslate(0);
				}
			}
		});

		translate_line();
		set_body_image();
		set_full_text();

		init_full_screen();
	}

	private void set_full_text()
	{
		TextView text = (TextView)findViewById(R.id.full_search_text);
		text.setText(N("FULL_SEARCH_TEXT"));

		String [] f = {
			N("FULL_TEXT_SHOW_SOME"),
			N("FULL_TEXT_SHOW_TITLE"),
			N("FULL_TEXT_SHOW_ALL"),
		};
		Spinner spin = new_spinner(R.id.full_search_set, f, GetFullTextShow()); 
		spin.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> arg0, View arg1, int idx, long arg3) {  
				SetFullTextShow(idx);
			}  
			public void onNothingSelected(AdapterView<?> arg0) {  
			}  
		}); 
	}

	private void set_body_image()
	{
		TextView use_body_image = (TextView)findViewById(R.id.use_body_image);
		use_body_image.setText(N("FW_FB_ENABLE_BODY_IMAGE"));

		m_body_image_box = (CheckBox)findViewById(R.id.body_image_box);
		m_body_image_box.setChecked(GetBodyImageFlag() == 0 ? false : true);

		m_body_image_box.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener(){ 
			@Override 
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) { 
				if (isChecked) {
					SetBodyImageFlag(1);
				} else {
					SetBodyImageFlag(0);
				}
			}
		});

		TextView select_body_image_text = (TextView)findViewById(R.id.select_body_image_text);
		select_body_image_text.setText(N("FW_FB_SELECT_BODY_IMAGE"));

		Button m_body_button = (Button)findViewById(R.id.select_body_image);
		m_body_button.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				startIntentForClass(200, FileBrowse.class);
			}
		});

		m_body_image_path = (TextView)findViewById(R.id.body_image_path);

		show_body_image_path();
	}

	private void set_one_text(int id, String s)
	{
		m_tmp_text = (TextView)findViewById(id);
		m_tmp_text.setText(N(s));
	}

	public void translate_line()
	{
		show_text = (TextView)findViewById(R.id.translate_show_text);
		show_line = (TextView)findViewById(R.id.translate_show_line);
		show_back = (ImageButton)findViewById(R.id.translate_show_line_back);
		show_forward = (ImageButton)findViewById(R.id.translate_show_line_forward);

		show_line.setText(GetTranslateShowLine() + "");

		show_back.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {
				int x = SetTranslateShowLine(-1);
				show_line.setText(x + "");
			}
		});

		show_forward.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {
				int x = SetTranslateShowLine(1);
				show_line.setText(x + "");
			}
		});
	}

	public Spinner new_spinner(int id, String [] m, int default_pos)
	{
		Spinner x = (Spinner) findViewById(id);

		set_spinner_adapter(x, m, default_pos);

		return x;
	}

	public void set_spinner_adapter(Spinner x, String [] m, int default_pos)
	{
		ArrayAdapter<String> xad = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, m);
		xad.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

		x.setAdapter(xad);
		x.setSelection(default_pos, true);
		x.setVisibility(View.VISIBLE); 
	}
	private void set_full_screen()
	{
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
	}

	private void exit_full_screen()
	{
		final WindowManager.LayoutParams attrs = this.getWindow().getAttributes();

		attrs.flags &= (~WindowManager.LayoutParams.FLAG_FULLSCREEN);
		this.getWindow().setAttributes(attrs);
		this.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
	}

	private void switch_full_screen()
	{
		if (WikiSwitchFullScreen() == 0) {
			exit_full_screen();
		} else {
			set_full_screen();
		}
	}

	private void init_full_screen()
	{
		if (WikiGetFullScreenFlag() == 1) {
			set_full_screen();
		}
	}

	private void show_short_msg(String msg)
	{
		Toast toast = Toast.makeText(FastwikiSetting.this, msg, Toast.LENGTH_LONG); 
		toast.show();
	}

	protected void startIntentForClass(int requestCode, java.lang.Class<?> cls){
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
		intent.setClass(this, cls);
		startActivityForResult(intent, requestCode);
	}

	private void show_body_image_path()
	{
		String m = GetBodyImagePath();

		m_body_image_path.setText(m);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Bundle bundle = null;
		if(data == null || (bundle = data.getExtras()) == null)
			return;

		if (requestCode == 200) {
			String path = bundle.getString("body_path");
			if (!path.equals("")) {
				SetBodyImagePath(path);
				show_body_image_path();
			}
		}
	}

	public native int GetMutilLangListMode();
	public native int SetMutilLangListMode(int mode);

	public native int GetTranslateShowLine();
	public native int SetTranslateShowLine(int x);

	public native int GetUseLanguage();
	public native int SetUseLanguage(int x);

	public native int GetHomePageFlag();
	public native int SetHomePageFlag(int idx);

	public native int GetRandomFlag();
	public native int SetRandomFlag(int idx);

	public native int GetNeedTranslate();
	public native int SetNeedTranslate(int f);

	public native String TranslateDefault();
	public native int SetTranslateDefault(String lang);
	public native String[] WikiLangList();
	public native String N(String name);

	public native int WikiSwitchFullScreen();
	public native int WikiGetFullScreenFlag();

	public native int GetBodyImageFlag();
	public native int SetBodyImageFlag(int f);

	public native String GetBodyImagePath();
	public native int SetBodyImagePath(String m);

	public native int GetFullTextShow();
	public native int SetFullTextShow(int idx);
}
