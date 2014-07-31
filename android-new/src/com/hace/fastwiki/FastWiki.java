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

/*
import android.view.Menu;  
import android.view.SubMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
*/

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

import com.actionbarsherlock.app.SherlockFragmentActivity;
import com.actionbarsherlock.app.ActionBar;
import com.actionbarsherlock.view.ActionMode;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.SubMenu;
import com.actionbarsherlock.view.MenuItem;

public class FastWiki extends SherlockFragmentActivity {
	WebView my_view;  
	//LinearLayout myUrlEntry;  
	RelativeLayout layout;

	EditText myUrl;  

	EditText find_text;
	String m_last_find = null;

	WebSettings websettings;

	//ImageButton forward;
	ImageButton Delete;

	ImageButton bookmark;
	ImageButton next_page;

	ImageButton history;
	ImageButton content;

	String last_string = "";
	String base_url = "";

	String [] m_lang = null;

	boolean have_network = false;
	int init_flag = 0;
	int full_screen_flag = 0;
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

	private int last_time = 0;
	private int back_time = 0;

	private boolean m_isfull = false;

	private ViewGroup customViewForDictView;

	View trans_view = null;
	private PopupWindow trans_popup = null;
	TextView m_trans;

	private int change_flag = 0;
	private boolean m_list_view_flag = false;

	ActionBar m_ab;
	ListView m_list_view = null;

	private int not_found = 0;
	private String [] m_list_color = null;
	private int m_curr_color_mode = 0;

	String [][] m_change_items = null;
	int m_change_items_total = 0;

	private Menu m_menu = null;
	int add_fav_flag = 0;

	/*
	private Handler m_handler;
	private Thread m_thread;
	*/

	/** Called when the activity is first created. */  

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		//this.requestWindowFeature(Window.FEATURE_NO_TITLE);

		setContentView(R.layout.main);  

		m_ab = getSupportActionBar();
		m_ab.setDisplayHomeAsUpEnabled(false);
		m_ab.setDisplayShowCustomEnabled(true);
		m_ab.setDisplayShowTitleEnabled(false);
		m_ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.search_edit, null);
		m_ab.setCustomView(customViewForDictView);

		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);  
		imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);

		init_flag = WikiInit();
		base_url = WikiBaseUrl();

		m_curr_color_mode = WikiCurrColorMode();

		my_view = (WebView)findViewById(R.id.mybrowser);  
		my_view.setScrollBarStyle(View.SCROLLBARS_INSIDE_OVERLAY);

		m_list_view = (ListView)findViewById(R.id.listview);
		m_list_color = WikiListColor();
		m_curr_color_mode = WikiCurrColorMode();

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

		websettings.setRenderPriority(RenderPriority.HIGH);
		websettings.setJavaScriptEnabled(true); 
		my_view.addJavascriptInterface(this, "android"); 
		websettings.setAllowFileAccess(false); 

		Delete = (ImageButton)customViewForDictView.findViewById(R.id.delete01);
		Delete.setOnClickListener(delete01_listener);

		myUrl = (EditText)customViewForDictView.findViewById(R.id.url);  

		my_view.setWebViewClient(new WebViewClient() {
			public void onPageFinished(WebView view, String url) {
				super.onPageFinished(view, url);
				if (load_js) {
					my_view.loadUrl("javascript:get_data()");
					load_js = false;
				}
				WikiLoadOnePageDone(); // TODO 
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
					//set_bookmark_icon();
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
		
		change_to_webview();
		homepage();
		/* TODO 512 */
		m_change_items = new String [512][2];

		myUrl.addTextChangedListener(new TextWatcher(){
			public void afterTextChanged(Editable s) {
			}
			public void beforeTextChanged(CharSequence s, int start, int count, int after){
			}
			public void onTextChanged(CharSequence s, int start, int before, int count){
				if (change_flag == 0) {
					on_change();
				}
			}

		});

		myUrl.setOnKeyListener(new OnKeyListener() { 
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER) {
					if (change_flag == 0) {
						on_change();
					}
					return true;
				}
				return false;
			}
		});

		full_screen_flag = WikiGetFullScreenFlag();
		init_full_screen();
	}

	public void set_title(String title)
	{
		change_flag = 1;
		last_string = title;

		myUrl.setText(title);
		myUrl.setSelection(title.length());
		change_flag = 0;
	}

	private void change_to_list_view()
	{
		my_view.setVisibility(View.GONE);
		m_list_view.setVisibility(View.VISIBLE);
		m_list_view_flag = true;
	}

	private void change_to_webview()
	{
		my_view.setVisibility(View.VISIBLE);
		m_list_view.setVisibility(View.GONE);
		m_list_view_flag = false;
	}

	private ImageButton.OnClickListener delete01_listener = new ImageButton.OnClickListener() {
		public void onClick(View arg0) { 
			set_title("");
			imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
		}
	}; 

	public void on_change()
	{
		MatchAdapter adapter;
		String m = myUrl.getText().toString();

		if (m.equals(""))
			return;

		String [] x = WikiMatch(m);
		String [] y = WikiMatchLang();

		m_change_items_total = x.length;

		if (x.length == 0) {
			not_found = 1;
			m_change_items[0][0] = "Not found";
			m_change_items[0][1] = "";
		} else {
			not_found = 0;
			for (int i = 0; i < x.length; i++) {
				m_change_items[i][0] = x[i];
				m_change_items[i][1] = y[i];
			}
		}
		change_to_list_view();

		adapter = new MatchAdapter(this, m_change_items);

		m_list_view.setAdapter(adapter);

		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				if (not_found == 0) {
					my_view.loadDataWithBaseURL(base_url, WikiViewIndex(pos), "text/html", "utf8", "");
					load_js = true;
					change_to_webview();
					set_title(WikiCurrTitle());
					imm.hideSoftInputFromWindow(myUrl.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
				}
			}
		});

		return;
	}

	protected void init_trans_popup_window(String s)
	{
		trans_view = getLayoutInflater().inflate(R.layout.trans, null, false);
		trans_popup = new PopupWindow(trans_view, LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT, true);

		m_trans = (TextView)trans_view.findViewById(R.id.trans);
		m_trans.setText(Html.fromHtml(TransLate(s)));

		trans_view.setOnTouchListener(new OnTouchListener() {
			public boolean onTouch(View v, MotionEvent event) {
				if (trans_popup != null && trans_popup.isShowing()) {
					trans_popup.dismiss();
					trans_popup = null;
				}
				return false;
			}
		});
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
		if (full_screen_flag != WikiGetFullScreenFlag()) {
			full_screen_flag = WikiGetFullScreenFlag();
			if (full_screen_flag == 1) {
				set_full_screen();
			} else {
				exit_full_screen();
			}
		}
	}

	private void init_full_screen()
	{
		if (WikiGetFullScreenFlag() == 1) {
			set_full_screen();
		}
	}


	public void translate(String m)
	{
		init_trans_popup_window(m);
		trans_popup.showAtLocation(my_view, Gravity.CENTER, 0, -80);
	}

	public void homepage() {
		load_js = true;
		my_view.loadDataWithBaseURL(base_url, IndexMsg(), "text/html", "utf8", "");
		set_title(WikiCurrTitle());
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getSupportMenuInflater().inflate(R.menu.fw_menu, menu);
		this.m_menu = menu;

		menu.findItem(R.id.about).setTitle(N("FW_MENU_ABOUT"));
		menu.findItem(R.id.help).setTitle(N("FW_MENU_HELP"));
		menu.findItem(R.id.quit).setTitle(N("FW_MENU_EXIT"));
		menu.findItem(R.id.display_top).setTitle(N("FW_MENU_TOP"));
		menu.findItem(R.id.forward).setTitle(N("FW_MENU_FORWARD"));
		menu.findItem(R.id.back).setTitle(N("FW_MENU_BACK"));
		menu.findItem(R.id.zoom_in).setTitle(N("FW_MENU_ZOOM_IN"));
		menu.findItem(R.id.zoom_out).setTitle(N("FW_MENU_ZOOM_OUT"));
		menu.findItem(R.id.shuffle).setTitle(N("FW_MENU_RANDOM"));
		menu.findItem(R.id.read_mode).setTitle(N("FW_MENU_READ_MODE"));
		menu.findItem(R.id.day_mode).setTitle(N("FW_MENU_DAY_MODE"));
		menu.findItem(R.id.night_mode).setTitle(N("FW_MENU_NIGHT_MODE"));
		menu.findItem(R.id.add_to_fav).setTitle(N("FW_MENU_ADD_TO_FAV"));

		return super.onCreateOptionsMenu(menu);
	}

	/*
	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		//menu.findItem(R.id.add_to_fav).setCheckable(false);
		return super.onPrepareOptionsMenu(menu);
	}
	*/

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();
		if (m_list_view_flag) {
			if (id == R.id.zoom_in || id == R.id.zoom_out || id == R.id.shuffle
					|| id == R.id.display_top || id == R.id.add_to_fav) {
				return true;
			}
		}
		switch (item.getItemId()) {
			case R.id.library:
				startIntentForClass(1, Library.class);
				return true;
			case R.id.setting:
				//startIntentForClass(0, SettingFrame.class);
				startIntentForClass(0, FastwikiSetting.class);
				return true;
			case R.id.history:
				/*
				if (add_fav_flag == 0) {
					m_menu.findItem(R.id.add_to_fav).setIcon(R.drawable.ic_add_to_fav_disable);
				} else {
					m_menu.findItem(R.id.add_to_fav).setIcon(R.drawable.ic_add_to_fav);
				}
				add_fav_flag = (add_fav_flag == 0) ? 1 : 0;
				*/
				startIntentForClass(99, History.class);
				return true;
			case R.id.favorites:
				startIntentForClass(102, Favorite.class);
				return true;

			case R.id.zoom_in:
				set_font_size(1);
				return true;
			case R.id.zoom_out:
				set_font_size(-1);
				return true;

			case R.id.shuffle:
				my_view.loadDataWithBaseURL(base_url, WikiRandom(), "text/html", "utf8", "");
				set_title(WikiCurrTitle());
				return true;

			case R.id.display_top:
				goto_display_top();
				return true;

			case R.id.add_to_fav:
				if (AddFavorite() == 1) {
					show_short_msg("\"" + WikiCurrTitle() + "\" " + N("ADD_FAV_MSG"));;
				}
				break;

			case R.id.quit:
				openExitDialog();
				break;

			case R.id.about:
				startIntentForClass(103, FastWikiAbout.class);
				break;

			case R.id.day_mode:
				if (m_curr_color_mode != 0) {
					m_curr_color_mode = WikiSetColorMode(0);
					m_list_color = WikiListColor();
					my_view.loadDataWithBaseURL(base_url, WikiCurr(), "text/html", "utf8", "");
					load_js = true;
					set_title(WikiCurrTitle());
				}
				break;

			case R.id.night_mode:
				if (m_curr_color_mode != 1) {
					m_curr_color_mode = WikiSetColorMode(1);
					m_list_color = WikiListColor();
					my_view.loadDataWithBaseURL(base_url, WikiCurr(), "text/html", "utf8", "");
					load_js = true;
					set_title(WikiCurrTitle());
				}
				break;

			default:
				return super.onOptionsItemSelected(item);
		}
		return true;
	}

	private void goto_display_top()
	{
		int last_pos = WikiLastPos();
		int last_height = WikiLastHeight();

		my_view.loadUrl("javascript:goto_top(" + last_pos + "," + last_height + ")");
	}

	protected void startIntentForClass(int requestCode, java.lang.Class<?> cls){
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
		intent.setClass(this, cls);
		startActivityForResult(intent, requestCode);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
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
				my_view.loadDataWithBaseURL(base_url, t[1], "text/html", "utf8", "");
				// TODO 
				set_title(WikiCurrTitle());
				load_js = true;
				//set_bookmark_icon();
			}/* else {
				set_title(t[1]);
				on_change();
			}
			*/
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	/* for android 4.0.x */
	@Override
	public boolean dispatchTouchEvent(MotionEvent event) {
		int act = event.getAction();

		if (act == MotionEvent.ACTION_MOVE) {
			my_view.loadUrl("javascript:set_pos()");
		}
		return super.dispatchTouchEvent(event);
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		int act = event.getAction();

		if (act == MotionEvent.ACTION_MOVE) {
			my_view.loadUrl("javascript:set_pos()");
		}

		return super.onTouchEvent(event);
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
	private void show_short_msg(String msg)
	{
		Toast toast = Toast.makeText(FastWiki.this, msg, Toast.LENGTH_SHORT); 
		toast.show();
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
								//TODO SetRandom();
							}
						}
					});
		}

		alert.show();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Bundle bundle = null;

		if (requestCode == 0) {
			switch_full_screen();
			return;
		}

		if(data == null || (bundle = data.getExtras()) == null)
			return;

		if (requestCode == 99) {
			int pos = bundle.getInt("history_pos");
			String t = bundle.getString("history_title");
			String m =  WikiViewHistory(pos);
			if (m.equals("")) {
				show_short_msg("Not found: " + t);
			} else {
				my_view.loadDataWithBaseURL(base_url, m, "text/html", "utf8", "");
				set_title(WikiCurrTitle());
				load_js = true;
				change_to_webview();
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
				change_to_webview();
			}
		}
	}

	private class MatchAdapter extends BaseAdapter
	{
		private LayoutInflater mInflater;
		private String [][] m_item;

		public MatchAdapter(Context context, String [][] it)
		{
			mInflater = LayoutInflater.from(context);

			m_item = it;
		}

		@Override
		public int getCount()
		{ 
			if (m_change_items_total == 0)
				return 1;

			return m_change_items_total; //m_item.length;
		} 

		@Override
		public Object getItem(int position)
		{
			return m_item[position];
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
				convertView = mInflater.inflate(R.layout.match_one, null);
				holder = new ViewHolder();
				holder.title  = (TextView)convertView.findViewById(R.id.match_title);
				holder.lang = (TextView)convertView.findViewById(R.id.match_lang);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			holder.title.setText(Html.fromHtml(m_item[position][0].toString()));
			holder.title.setBackgroundColor(Color.parseColor(m_list_color[0]));
			holder.title.setTextColor(Color.parseColor(m_list_color[1]));

			if (m_item[position][1].equals("")) {
				holder.lang.setVisibility(View.GONE);
			} else {
				holder.lang.setVisibility(View.VISIBLE);
				holder.lang.setText(Html.fromHtml(m_item[position][1].toString()));
				holder.lang.setBackgroundColor(Color.parseColor(m_list_color[0]));
				holder.lang.setTextColor(Color.parseColor(m_list_color[1]));
			}

			return convertView;
		}
		private class ViewHolder
		{
			TextView title;
			TextView lang;
		}
	}
	public native String [] WikiMatch(String key);
	public native int WikiMatchTotal();
	public native String [] WikiMatchLang();
	public native String [] WikiListColor();

	public native String IndexMsg();
	public native String WikiRandom();
	public native String WikiCurrTitle();

	public native int WikiInit();
	public native int WikiReInit();
	public native int WikiExit();

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

	public native void WikiSleep(int usec);
	public native void LOG(String str);

	public native int GetTimeOfDay();

	public native int WikiSetHideMenuFlag();
	public native int WikiGetHideMenuFlag();

	public native int WikiColorMode();
	public native int WikiSetColorMode(int idx);
	public native int WikiCurrColorMode();

	public native String WikiViewIndex(int idx);
	public native int WikiGetFullScreenFlag();
	public native String WikiViewHistory(int pos);
	public native int WikiLoadOnePageDone();

	public native String WikiViewFavorite(int pos);
	public native String TransLate(String str);

	static {
		System.loadLibrary("fastwiki");
	}
}
