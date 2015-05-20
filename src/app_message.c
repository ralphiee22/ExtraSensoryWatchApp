#include <pebble.h>

static Window *window;	
static TextLayer *s_output_layer;	
static char temp[24];

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 42
};

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

// Write message to buffer & send
void send_message(char *str){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_KEY, str);
	
	dict_write_end(iter);
  	app_message_outbox_send();
}

// data handler to recieve accel data from watch
static void data_handler(AccelData *data, uint32_t num_samples) {
  if(num_samples == 0){
    return;
  }
   AccelData* d = data;
  
//   // Prepare dictionary
   DictionaryIterator *iterator;
   app_message_outbox_begin(&iterator);
    
   for(uint8_t i = 0; i < num_samples; i++, d++) {
      snprintf(temp, 24, "%d,%d,%d", d->x, d->y, d->z);
      dict_write_cstring(iterator, i , temp);     
   }  
  //dict_write_cstring(iterator, 1, s_buffer);
   app_message_outbox_send();
  }

// void turnOnAccelCollection()
//   {
//   // subscribe to the accel data 
//   uint32_t num_samples = 25;
//   accel_data_service_subscribe(num_samples, data_handler);
//   accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
// }

// Called when a message is received from iPhone
static void in_received_handler(DictionaryIterator *iterator, void *context) {
    // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch (t->key) {
      case 1:
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY DATA: %d", (int)t->key);
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %s", t->value->cstring);
      if(strcmp(t->value->cstring, "TURN OFF") == 0)
      {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Function went to unsubscribe."); 
         APP_LOG(APP_LOG_LEVEL_DEBUG, "UPDATES WENT TO 0"); 

//         accel_data_service_unsubscribe();
          accel_service_set_samples_per_update(0);
    }
      if(strcmp(t->value->cstring, "TURN ON") == 0)
      {
         APP_LOG(APP_LOG_LEVEL_DEBUG, "UPDATES WENT TO 25"); 
        accel_service_set_samples_per_update(25);
    }
        break;
      case 2:
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY DATA: %d", (int)t->key);
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %s", t->value->cstring);
      text_layer_set_text(s_output_layer, t->value->cstring);
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
    // outgoing message was delivered
    APP_LOG(APP_LOG_LEVEL_DEBUG, "DICTIONARY SENT SUCCESSFULLY!");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Size of Dict: %d", (int)dict_size(sent));

}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    // outgoing message failed
    APP_LOG(APP_LOG_LEVEL_DEBUG, "DICTIONARY NOT SENT! ERROR!");
   //APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
}

// when message is recieved, click up button to confirm activity
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "UP Button Pressed!");
  text_layer_set_text(s_output_layer, "Confirmed as yes! Waiting for new message...");
  send_message("YES");
}

// when message is recieved, click down button to confirm it was not activity
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "DOWN Button Pressed!");
  text_layer_set_text(s_output_layer, "Not now! Waiting for new message...");
  send_message("NO");
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// Called when an incoming message from iPhone is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message was dropped!");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(5, 0, window_bounds.size.w - 5, window_bounds.size.h));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, "No message recieved yet.");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  
  // add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
}

void init(void) {
  // create main window element and assign to pointr
	window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_set_click_config_provider(window, click_config_provider);
      
  // Show the Window on the watch, with animated=true
	window_stack_push(window, true);
  
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
  app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
  
  // subscribe to the accel data 
  uint32_t num_samples = 25;
  accel_data_service_subscribe(num_samples, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);

	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	}

void deinit(void) {
	app_message_deregister_callbacks();
  accel_data_service_unsubscribe();
  app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	window_destroy(window);
}

int main( void ) {
	init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
	app_event_loop();
	deinit();
}