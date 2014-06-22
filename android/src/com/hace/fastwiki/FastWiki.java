/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;

import java.io.File;
import java.lang.reflect.Method;
import android.util.Log;

import android.app.Activity;  
import android.app.AlertDialog;  
import android.content.DialogInterface;  

import android.os.Bundle;  
import android.os.Handler;
import android.os.Message;
import android.os.Build;
 
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.view.Menu;  
import android.view.SubMenu;
import android.view.MenuInflater;
import android.view.MenuItem;

import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.KeyEvent;
import android.view.Display;
import android.view.WindowManager;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;
import android.view.View.OnKeyListener;
import android.view.LayoutInflater;
import android.view.Gravity;
import android.widget.ScrollView;

import android.net.Uri;

import android.webkit.WebView;
import android.webkit.CacheManager;
import android.webkit.WebSettings;
import android.webkit.WebSettings.LayoutAlgorithm;  
import android.webkit.WebSettings.RenderPriority;
import android.webkit.WebViewClient;
import android.webkit.ConsoleMessage;
import android.webkit.ConsoleMessage.MessageLevel;
import android.webkit.WebChromeClient;
import android.graphics.Bitmap;

import android.widget.ImageButton;
import android.widget.Button;
import android.widget.ImageView;
//import android.widget.ZoomButton;
//import android.widget.ZoomControls;
//import android.widget.ZoomButtonsController;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.RelativeLayout;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Toast;

import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;

import android.widget.RadioButton;  
import android.widget.RadioGroup;

import java.lang.reflect.Field;

import android.content.Context;
import android.content.Intent;

import android.text.TextWatcher;
import android.text.Editable;
import android.text.InputType;
import android.text.Html;
import android.graphics.Color; 

public class FastWiki extends Activity {
	WebView my_view;  
	LinearLayout myUrlEntry;  
	LinearLayout fw_menu;
	RelativeLayout layout;

	EditText myUrl;  

	EditText find_text;
	String m_last_find = null;

	WebSettings websettings;

	ImageButton forward;
	ImageButton Delete;

	ImageButton bookmark;
	ImageButton next_page;

	ImageButton history;
	ImageButton content;

	int change_flag = 0;
	String last_string = "";
	String base_url = "";

	String [] m_lang = null;

	boolean have_network = false;
	int init_flag = 0;
	InputMethodManager imm;

	Activity m_activity = this;

	String m_delete_bookmark_key = "";

	private PopupWindow library_popup = null;
	private PopupWindow content_popup = null;
	private PopupWindow find_popup = null;

	View library_view = null;
	View content_view = null;
	View find_view = null;

	String m_register_msg = "";

	private boolean load_js = false;
	private boolean is_loading = false;

	private ListView m_list_view = null;
	private boolean m_list_view_flag = false;
	private String [] m_list_color = null;

	private int last_time = 0;
	private int back_time = 0;

	private boolean m_isfull = false;

	private int m_curr_color_mode = 0;

	/*
	private Handler m_handler;
	private Thread m_thread;
	*/

	/** Called when the activity is first created. */  

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		this.requestWindowFeature(Window.FEATURE_NO_TITLE);

		setContentView(R.layout.main);  

		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);  
		imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);

		init_flag = WikiInit();
		base_url = WikiBaseUrl();

		m_list_color = WikiListColor();
		m_curr_color_mode = WikiCurrColorMode();

		m_list_view = (ListView)findViewById(R.id.listview);
		my_view = (WebView)findViewById(R.id.mybrowser);  
		my_view.setScrollBarStyle(View.SCROLLBARS_INSIDE_OVERLAY);

		websettings = my_view.getSettings();  

		websettings.setSupportZoom(false);
		websettings.setCacheMode(WebSettings.LOAD_NO_CACHE);

		try { 
			for (Method m : CacheManager.class.getDeclaredMethods()){
				if(m.getName().equals("setCacheDisabled")){
					m.setAccessible(true);
					m.invoke(my_view, true); 
				}
			}
		} catch (Throwable e) {
		}

		//websettings.setBuiltInZoomControls(true); 

		//websettings.setDefaultZoom();
		//websettings.setUseWideViewPort(false);
		/* TODO need add to setting */
		//websettings.setLayoutAlgorithm(LayoutAlgorithm.NARROW_COLUMNS);
		
		//websettings.setLayoutAlgorithm(LayoutAlgorithm.SINGLE_COLUMN);
		///websettings.setLayoutAlgorithm(LayoutAlgorithm.NORMAL);

		websettings.setRenderPriority(RenderPriority.HIGH);
		websettings.setJavaScriptEnabled(true); 
		my_view.addJavascriptInterface(this, "android"); 
		websettings.setAllowFileAccess(false); 

		forward = (ImageButton)findViewById(R.id.forward);
		forward.setOnClickListener(myGoButtonOnClickListener);

		Delete = (ImageButton)findViewById(R.id.delete01);
		Delete.setOnClickListener(delete01_listener);
		
		myUrlEntry = (LinearLayout)findViewById(R.id.UrlEntry);  
		myUrlEntry.setVisibility(View.VISIBLE);  

		ImageButton ic_library = (ImageButton)findViewById(R.id.ic_library);
		ic_library.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				get_library_pop_window();
				library_popup.showAsDropDown(v);
			}
		});

		content = (ImageButton)findViewById(R.id.ic_content);
		content.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				get_content_pop_window();
				content_popup.showAsDropDown(v);
			}
		});

		history = (ImageButton)findViewById(R.id.ic_history);
		history.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View arg0) {  
				start_history_intent();
				/*
				set_title("");
				my_view.loadDataWithBaseURL(base_url, WikiHistory(), "text/html", "utf8", "");
				*/
			}
		});

		next_page = (ImageButton)findViewById(R.id.next_page);
		next_page.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View arg0) {  
				goto_next_page();
			}
		});

		bookmark = (ImageButton)findViewById(R.id.ic_add_to_fav);
		set_bookmark_icon();

		bookmark.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View arg0) {  
				String msg = "";
				if (IsFavorite() == 1) {
					msg = N("DEL_FAV_MSG") + " \"" + WikiCurrTitle() + "\"";
					DeleteFavorite();
				} else {
					msg = "\"" + WikiCurrTitle() + "\" " + N("ADD_FAV_MSG");
					AddFavorite();
				}
				show_short_msg(msg);
				set_bookmark_icon();
			}
		});

		fw_menu = (LinearLayout)findViewById(R.id.fw_menu);  
		hide_or_show_menu();
		
		layout = (RelativeLayout)findViewById(R.id.main2);

		myUrl = (EditText)findViewById(R.id.url);  

		myUrl.addTextChangedListener(new TextWatcher(){
			public void afterTextChanged(Editable s) {
			}
			public void beforeTextChanged(CharSequence s, int start, int count, int after){
			}
			public void onTextChanged(CharSequence s, int start, int before, int count){
				if (change_flag == 0)
					on_change();
			}

		});

		myUrl.setOnKeyListener(new OnKeyListener() { 
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER) {
					if (change_flag == 0)
						on_change();
					return true;
				}
				return false;
			}
		});

		/* TODO */
		/*
		my_view.setOnTouchListener(new View.OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				goto_next_page();
				return false;
			}
		});
		*/

		my_view.setWebViewClient(new WebViewClient() {
			public void onPageFinished(WebView view, String url) {
				super.onPageFinished(view, url);
				if (load_js) {
					my_view.loadUrl("javascript:get_data()");
					load_js = false;
				}
				WikiLoadOnePageDone(); /* TODO */
			}

			public void onProgressChanged(WebView view, int progress) {
				//FastWiki.this.setProgress(progress * 100);
			}

			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				imm.hideSoftInputFromWindow(myUrl.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);

				//websettings.setJavaScriptEnabled(false); 

				String [] tmp = WikiParseUrl(url);

				if (tmp[0].equals("0")) {
					NotFound(tmp[1]);
					return true;
				} else if (tmp[0].equals("1")) {
					load_js = true;
					my_view.loadDataWithBaseURL(base_url, tmp[2], "text/html", "utf8", "");
					set_title(tmp[1]);
					set_bookmark_icon();
					return true;
				} else if (tmp[0].equals("2")) {
					Uri uri = Uri.parse(tmp[1]);
					Intent it  = new Intent(Intent.ACTION_VIEW, uri);
					startActivity(it); 
					return true;
				}
				return true;
			}
		}); 

		/*
		my_view.setWebChromeClient(new WebChromeClient() {
			@Override
			public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
				MessageLevel level = consoleMessage.messageLevel();
				if (level.ordinal() == 3 ) { // Error ordinal
					if(is_loading){
						my_view.stopLoading();
						//my_view.reload();
						my_view.loadDataWithBaseURL(base_url, WikiCurr(), "text/html", "utf8", "");
						is_loading = false;
						load_js = true;
					}
				}
				return false;
			}
		});
		*/

		change_to_webview();
		homepage();
		init_full_screen();

		/*
		ActionBar actionBar = SherlockFragment.getSupportActionBar();
		actionBar.show();
		*/
	}

	/*
	private void hide_menu()
	{
		myUrlEntry.setVisibility(View.GONE);  
		fw_menu.setVisibility(View.GONE);  
	}
	*/

	private void change_to_list_view()
	{
		my_view.setVisibility(View.GONE);
		fw_menu.setVisibility(View.GONE);
		forward.setVisibility(View.GONE);
		m_list_view.setVisibility(View.VISIBLE);
		m_list_view_flag = true;
	}

	private void change_to_webview()
	{
		my_view.setVisibility(View.VISIBLE);
		forward.setVisibility(View.VISIBLE);
		m_list_view.setVisibility(View.GONE);
		hide_or_show_menu();
		m_list_view_flag = false;
	}

	private void hide_or_show_menu()
	{
		if (WikiGetHideMenuFlag() == 0) {
			fw_menu.setVisibility(View.VISIBLE);
		} else {
			fw_menu.setVisibility(View.GONE);
		}
	}

	private void get_library_pop_window() {

		if (library_popup == null) {
			init_library_popup_window();
		} else {
			library_popup.dismiss();
		}
	}

	private void get_content_pop_window() {

		if (content_popup == null) {
			init_content_popup_window();
		} else {
			content_popup.dismiss();
		}
	}

	protected void init_content_popup_window()
	{
		content_view = getLayoutInflater().inflate(R.layout.content, null, false);
		content_popup = new PopupWindow(content_view, LayoutParams.WRAP_CONTENT,  
				                     320, true); //LayoutParams.WRAP_CONTENT, true);

		ImageButton zoom_in = (ImageButton)content_view.findViewById(R.id.zoom_in);
		zoom_in.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				set_font_size(1);
			}
		});

		ImageButton zoom_out = (ImageButton)content_view.findViewById(R.id.zoom_out);
		zoom_out.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				set_font_size(-1);
			}
		});

		ImageButton search = (ImageButton)content_view.findViewById(R.id.search);
		search.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				content_popup.dismiss();
				content_popup = null;

				View v2 = getLayoutInflater().inflate(R.layout.main, null, false);
				init_find_popup_window();
				find_popup.showAtLocation(v2, Gravity.CENTER, 0, -80);
			}
		});

		ImageButton shuffle = (ImageButton)content_view.findViewById(R.id.shuffle);
		shuffle.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				my_view.loadDataWithBaseURL(base_url, WikiRandom(), "text/html", "utf8", "");
				set_title(WikiCurrTitle());
				set_bookmark_icon();
			}
		});

		ImageButton colormode = (ImageButton)content_view.findViewById(R.id.colormode);
		if (m_curr_color_mode == 0) {
			colormode.setImageDrawable(getResources().getDrawable(R.drawable.night_mode));
		} else {
			colormode.setImageDrawable(getResources().getDrawable(R.drawable.day_mode));
		}
		colormode.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				WikiColorMode();
				m_curr_color_mode = WikiCurrColorMode();
				m_list_color = WikiListColor();
				my_view.loadDataWithBaseURL(base_url, WikiCurr(), "text/html", "utf8", "");
				load_js = true;
				set_title(WikiCurrTitle());
				set_bookmark_icon();
			}
		});

		content_view.setOnTouchListener(new OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub
				if (content_popup != null && content_popup.isShowing()) {
					content_popup.dismiss();
					content_popup= null;
				}
				return false;
			}
		});
	}

	protected void init_find_popup_window()
	{
		find_view  = getLayoutInflater().inflate(R.layout.find, null, false);
		find_popup = new PopupWindow(find_view, LayoutParams.WRAP_CONTENT,  
				                     LayoutParams.WRAP_CONTENT, true);

		find_text = (EditText)find_view.findViewById(R.id.find_text);
		ImageButton find_prev = (ImageButton)find_view.findViewById(R.id.find_prev);
		ImageButton find_next = (ImageButton)find_view.findViewById(R.id.find_next);

		if (m_last_find != null) {
			find_text.setText(m_last_find);
			//FindFirst();
		}

		find_text.setOnKeyListener(new OnKeyListener() { 
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER) {
					FindFirst();
					return true;
				}
				return false;
			}
		});

		find_prev.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View arg0) {  
				FindPrev();
			}
		});

		find_next.setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View arg0) {  
				FindNext();
			}
		});

		find_view.setOnTouchListener(new OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub
				if (find_popup != null && find_popup.isShowing()) {
					find_popup.dismiss();
					find_popup = null;
					FindChanle();
				}
				return false;
			}
		});
	}

	protected void init_library_popup_window()
	{
		library_view = getLayoutInflater().inflate(R.layout.library, null, false);
		RadioGroup group = (RadioGroup)library_view.findViewById(R.id.radio_group);
		ScrollView sv = (ScrollView) findViewById(R.id.scroll);

		String curr_lang = WikiCurrLang();
		m_lang  = WikiLangList();
		if (group != null ) {
			for (int i = 0; i < m_lang.length; i++) {
				 RadioButton rb  = new RadioButton(this);
				 rb.setId(i);
				 rb.setText(" " + m_lang[i] + "   ");
				 if (curr_lang.equals(m_lang[i])) {
					 rb.setChecked(true);
				 }
				 group.addView(rb);
			}
			group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
				public void onCheckedChanged(RadioGroup group, int checkedId) {
					RadioButton rb = (RadioButton)library_view.findViewById(checkedId);
					WikiInitLang(m_lang[checkedId]);
					String [] tmp = WikiParseUrl("__curr__");
					if (tmp[0].equals("1")) {
						load_js = true;
						my_view.loadDataWithBaseURL(base_url, tmp[2], "text/html", "utf8", "");
						set_title(tmp[1]);
						set_bookmark_icon();
					}
				}
			}); 
		}

		int n = m_lang.length * 100;
		if (n > 400)
			n = 400;
		library_popup = new PopupWindow(library_view, LayoutParams.WRAP_CONTENT,  
				                     n, true); //LayoutParams.WRAP_CONTENT, true);
		/* TODO m_lang.length */

		//popupWindow.setAnimationStyle(R.style.AnimationFade);
		library_view.setOnTouchListener(new OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub
				if (library_popup != null && library_popup.isShowing()) {
					library_popup.dismiss();
					library_popup = null;
				}
				return false;
			}
		});
	}

	public void FindPrev()
	{
		String txt = find_text.getText().toString();

		if (m_last_find == null || !txt.equals(m_last_find)) {
			FindFirst();
			return;
		}

		my_view.findNext(false);
	}

	public void FindNext()
	{
		String txt = find_text.getText().toString();

		if (m_last_find == null || !txt.equals(m_last_find)) {
			FindFirst();
			return;
		}

		my_view.findNext(true);
	}

	public void FindFirst()
	{
		String txt = find_text.getText().toString();
		if (txt.equals("")) {
			return;
		}
		my_view.findAll(txt);
		m_last_find = txt;

		FindFlag(true);
	}

	public void FindChanle()
	{
		FindFlag(false);
	}

	public void FindFlag(boolean flag)
	{
		try{  
			for (Method m : WebView.class.getDeclaredMethods()){
				if(m.getName().equals("setFindIsUp")){
					m.setAccessible(flag);
					m.invoke(my_view, flag);
					break;
				}
			}
		}catch(Exception ignored){
		}  
	}

	private ImageButton.OnClickListener delete01_listener
		= new ImageButton.OnClickListener() {
		public void onClick(View arg0) { 
			set_title("");
			imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
		}
	}; 

	private ImageButton.OnClickListener myGoButtonOnClickListener
		= new ImageButton.OnClickListener() {
		public void onClick(View arg0) {  
			String [] t = WikiForward();
			if (t[1].equals("")) {
				return;
			} else if (t[1].equals("1")) {
				show_short_msg(N("LAST_PAGE"));
				return;
			}

			if (t[0].equals("1")) {
				change_to_webview();
				my_view.loadDataWithBaseURL(base_url, t[1], "text/html", "utf8", "");
				/* TODO */
				set_title(WikiCurrTitle());
				load_js = true;
				set_bookmark_icon();
			} else {
				set_title(t[1]);
				on_change();
			}
		}  
	}; 

	public void set_title(String title)
	{
		change_flag = 1;
		last_string = title;

		myUrl.setText(title);
		myUrl.setSelection(title.length());

		change_flag = 0;
	}

	public void on_change()
	{
		MatchAdapter adapter;
		List <String> items;
		String m = myUrl.getText().toString();

		if (m.equals(""))
			return;

		items = new ArrayList<String>();

		String [] x = WikiMatch(m);

		for (int i = 0; i < x.length; i++) {
			items.add(x[i]);
		}

		change_to_list_view();
		adapter = new MatchAdapter(this, items);

		m_list_view.setAdapter(adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				my_view.loadDataWithBaseURL(base_url, WikiViewIndex(pos), "text/html", "utf8", "");
				load_js = true;
				change_to_webview();
				set_title(WikiCurrTitle());
				imm.hideSoftInputFromWindow(myUrl.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
			}
		});

		return;
	}

	public void homepage() {
		load_js = true;
		my_view.loadDataWithBaseURL(base_url, IndexMsg(), "text/html", "utf8", "");
		set_title(WikiCurrTitle());
		set_bookmark_icon();
		myUrlEntry.setVisibility(View.VISIBLE);  
	}

	@Override  
	public boolean onCreateOptionsMenu(Menu menu) {
		//menu.add(Menu.NONE, 0, 0, N("MENU_HISTORY"));

		menu.add(Menu.NONE, 5, 0, N("MENU_FAVORITE"));

		SubMenu submenu = menu.addSubMenu(Menu.NONE, 1, 1, N("MENU_OPTION"));
		submenu.add(Menu.NONE, 20, 0, N("MENU_ABOUT"));
		submenu.add(Menu.NONE, 21, 0, N("MENU_SEL_FOLDER"));
		submenu.add(Menu.NONE, 22, 0, N("MENU_SET_HOME_PAGE"));

		submenu.add(Menu.NONE, 23, 0, N("MENU_HIDE_SHOW_MENU"));
		submenu.add(Menu.NONE, 24, 0, N("SCREEN_FULL"));


		menu.add(Menu.NONE, 3, 3, N("MENU_EXIT"));

		return true;  
	}  

	@Override  
	public boolean onPrepareOptionsMenu(Menu menu) {  
		//myUrlEntry.setVisibility(View.VISIBLE);  
		return true;  
	}  

	@Override  
	public boolean onOptionsItemSelected(MenuItem item) {
		// TODO Auto-generated method stub  
		String url;
		switch (item.getItemId()) {
			case 1:
				break;
			case 20:
				wiki_about();
				break;
			case 3:
				openExitDialog();
				break;
			case 5:
				start_favorite();
				break;
			case 21:
				wiki_setting();
				break;
			case 22:
				wiki_random_page();
				break;
			case 23:
				WikiSetHideMenuFlag();
				hide_or_show_menu();
				break;
			case 24:
				switch_full_screen();
				break;
		}  
		//myUrlEntry.setVisibility(View.VISIBLE);  
		return true;  
	} 

	@Override  
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {  
		Bundle bundle = null;  
		if(data == null || (bundle = data.getExtras()) == null)
			return;

		if(requestCode == 99){  
			String dir = bundle.getString("file");
			if (IsValidDir(dir) == 1) {
				init_flag = WikiReInit();
				base_url = WikiBaseUrl();
				homepage();
			} else {
				select_folder_not_found(dir);
			}
		} else if (requestCode == 101) { 
			int pos = bundle.getInt("history_pos");
			String t = bundle.getString("history_title");
			String m =  WikiViewHistory(pos);
			if (m.equals("")) {
				show_short_msg("Not found: " + t);
			} else {
				my_view.loadDataWithBaseURL(base_url, m, "text/html", "utf8", "");
				set_title(WikiCurrTitle());
				load_js = true;
			}
		} else if (requestCode == 102) {
			int pos = bundle.getInt("favorite_pos");
			String t = bundle.getString("favorite_title");
			String m =  WikiViewFavorite(pos);
			if (m.equals("")) {
				show_short_msg("Not found: " + t);
			} else {
				my_view.loadDataWithBaseURL(base_url, m, "text/html", "utf8", "");
				set_title(WikiCurrTitle());
				load_js = true;
			}
		}
	}

	private void show_short_msg(String msg)
	{
		Toast toast = Toast.makeText(FastWiki.this, msg, Toast.LENGTH_SHORT); 
		toast.show();
	}

	private void wiki_random_page()
	{
		String msg = IsRandom() == 1 ? N("MENU_DATE_PAGE") : N("MENU_RANDOM_PAGE");
		alert_msg("", msg, N("EXIT_YES"), N("EXIT_NO"), 2);
	}

	private void wiki_setting()
	{
		Intent intent = new Intent(FastWiki.this, MyFileManager.class);  
		startActivityForResult(intent, 99);
	}

	private void wiki_about()
	{
		Intent intent = new Intent(FastWiki.this, FastWikiAbout.class);
		startActivityForResult(intent, 100);
	}

	private void start_history_intent()
	{
		Intent intent = new Intent(FastWiki.this, History.class);
		startActivityForResult(intent, 101);
	}

	private void start_favorite()
	{
		Intent intent = new Intent(FastWiki.this, Favorite.class);
		startActivityForResult(intent, 102);
	}

	private void select_folder_not_found(String dir)
	{
		alert_msg(N("SELECT_FOLDER_TITLE"), N("SELECT_FOLDER_MSG") + "\n" + dir,
				N("SELECT_FOLDER_OK"), "", 0);
	}

	private void NotFound(String title)
	{
		alert_msg(N("NOT_FOUND_TITLE") + ": " + title,
				N("NOT_FOUND_MSG"), N("NOT_FOUND_OK"), "", 0);
	}

	private void openExitDialog()  
	{  
		alert_msg(N("EXIT_TITLE"), "", N("EXIT_YES"), N("EXIT_NO"), 1);
	}  


	/*
	 * m_alert_value:
	 *   =1  exit
	 *   =2  switch random
	 */
	private void alert_msg(String title, String msg, String yes,
			String no, final int m_alert_value)
	{
		AlertDialog.Builder alert = new AlertDialog.Builder(this);;

		if (!title.equals(""))
			alert.setTitle(title);

		if (!msg.equals(""))
			alert.setMessage(msg);

		if (!no.equals("")) {
			alert.setNegativeButton(no,
					new DialogInterface.OnClickListener(){  
						public void onClick(DialogInterface dialoginterface, int i) {
						}
					});
		}

		if (!yes.equals("")) {
			alert.setPositiveButton(yes,
					new DialogInterface.OnClickListener(){  
						public void onClick(DialogInterface dialoginterface, int i) {
							if (m_alert_value == 1) {
								WikiExit();
								finish(); 
							} else if (m_alert_value == 2) {
								SetRandom();
							}
						}
					});
		}

		alert.show();
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (m_list_view_flag) {
				set_title(WikiCurrTitle());
				change_to_webview();
				return true;
			}
			String [] t = WikiBack();
			if (t[1].equals("")) {
				return true;
			} else if (t[1].equals("1")) {
				back_time = GetTimeOfDay();
				if (back_time - last_time > 150) {
					show_short_msg(N("FIRST_PAGE"));
					last_time = back_time;
				} else {
					WikiExit();
				}
				return true;
			}
			if (t[0].equals("1")) {
				change_to_webview();
				my_view.loadDataWithBaseURL(base_url, t[1], "text/html", "utf8", "");
				/* TODO */
				set_title(WikiCurrTitle());
				load_js = true;
				set_bookmark_icon();
			} else {
				set_title(t[1]);
				on_change();
			}
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	/* for android 4.0.x */
	@Override
	public boolean dispatchTouchEvent(MotionEvent event) {
		int act = event.getAction();

		if (act == MotionEvent.ACTION_MOVE || act == MotionEvent.ACTION_UP) {
			my_view.loadUrl("javascript:set_pos()");
		}

		return super.dispatchTouchEvent(event);
	}

	@Override
	public boolean onTouchEvent(final MotionEvent event) {
		int act = event.getAction();

		if (act == MotionEvent.ACTION_MOVE || act == MotionEvent.ACTION_UP) {
			my_view.loadUrl("javascript:set_pos()");
		}

		return true;
	}

	public void goto_back_page()
	{
		my_view.loadUrl("javascript:back_page()");
	}

	public void goto_next_page()
	{
		my_view.loadUrl("javascript:next_page()");
	}

	public void debug(String h)
	{
		LOG("h=" + h + "\n");
	}

	public void real_set_pos(int pos, int height, int screen_height)
	{
		if (pos > 0)
			WikiSetTitlePos(pos, height, screen_height);
	} 

	public void has_last_page()
	{
		show_short_msg(N("CONTENT_LAST_PAGE"));
	}

	public void set_font_size(int n)
	{
		int size = WikiSetFontSize(n);

		if (size > 0)
			my_view.loadUrl("javascript:set_font_size(" + size + ")");
	}

	public void set_bookmark_icon()
	{
		if (IsFavorite() == 1) {
			bookmark.setImageDrawable(getResources().getDrawable(R.drawable.ic_favorites));
		} else {
			bookmark.setImageDrawable(getResources().getDrawable(R.drawable.ic_add_to_fav_disable));
		}
	}

	 private class MatchAdapter extends BaseAdapter
	{
		private LayoutInflater mInflater;
		private List <String> m_item;

		public MatchAdapter(Context context, List<String> it)
		{
			mInflater = LayoutInflater.from(context);

			m_item = it;
		}

		@Override
		public int getCount()
		{ 
			return m_item.size();
		} 

		@Override
		public Object getItem(int position)
		{
			return m_item.get(position);
		}

		@Override
		public long getItemId(int position)
		{
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder;

			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.match, null);
				holder = new ViewHolder();
				holder.text = (TextView)convertView.findViewById(R.id.match_text);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			holder.text.setText(Html.fromHtml(m_item.get(position).toString()));
			holder.text.setBackgroundColor(Color.parseColor(m_list_color[0]));
			holder.text.setTextColor(Color.parseColor(m_list_color[1]));

			return convertView;
		}
		private class ViewHolder
		{
			TextView text;
		}
	}

    private void set_full_screen()
	{
         m_activity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				 	WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }

    private void exit_full_screen()
	{
          final WindowManager.LayoutParams attrs = m_activity.getWindow().getAttributes();

          attrs.flags &= (~WindowManager.LayoutParams.FLAG_FULLSCREEN);
          m_activity.getWindow().setAttributes(attrs);
          m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
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

	public native String IndexMsg();
	public native String WikiRandom();
	public native String WikiCurrTitle();
	public native String [] WikiMatch(String key);

	public native int WikiInit();
	public native int WikiReInit();
	public native int WikiExit();

	public native int IsRandom();
	public native int SetRandom();

	public native int IsFavorite();
	public native int AddFavorite();
	public native int DeleteFavorite();

	public native String WikiCurr();
	public native String [] WikiBack();
	public native String [] WikiForward();

	public native String N(String name);

	public native int IsValidDir(String dir);
	public native String WikiBaseUrl();

	public native int WikiSetFontSize(int n);

	public native String[] WikiParseUrl(String url);

	public native void WikiSetTitlePos(int pos, int height, int screen_height);
	public native int WikiLastPos();
	public native int WikiLastHeight();

	public native int WikiInitLang(String lang);
	public native String WikiCurrLang();

	public native void WikiSleep(int usec);
	public native void LOG(String str);

	public native String[] WikiLangList();

	public native int GetTimeOfDay();

	public native int WikiSetHideMenuFlag();
	public native int WikiGetHideMenuFlag();

	public native int WikiColorMode();
	public native int WikiCurrColorMode();

	public native String WikiViewIndex(int idx);
	public native int WikiSwitchFullScreen();
	public native int WikiGetFullScreenFlag();
	public native String WikiViewHistory(int pos);
	public native int WikiLoadOnePageDone();
	public native String [] WikiListColor();

	public native String WikiViewFavorite(int pos);

	static {
		System.loadLibrary("fastwiki");
	}
}
