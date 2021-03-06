#include <pebble.h>
#define WINDOW_HEIGHT 168
#define WINDOW_WIDTH 144  
#define TEXTBOX_HEIGHT 2000
  
static Window *window;	
static TextLayer *s_output_layer;	
static TextLayer *static_messages;
static char temp[24];
static PropertyAnimation *s_box_animation;

static void animate_question();
static int number_of_pixels;

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

void anim_stopped_handler(Animation *animation, bool finished, void *context) {
  // Free the animation
  property_animation_destroy(s_box_animation);

  // Schedule the reverse animation, unless the app is exiting
  if (finished) {
    number_of_pixels = -number_of_pixels;
    animate_question(number_of_pixels);
  }
}

void animate_question(int pixels_to_scroll_by) {
  int len = pixels_to_scroll_by;
  if(pixels_to_scroll_by > 0) {
    len = pixels_to_scroll_by + 24;
  }
  else  {
    len = pixels_to_scroll_by - 24;
  }
  GRect start_frame = GRect(0, (pixels_to_scroll_by < 0 ? 0 : -len), WINDOW_WIDTH, TEXTBOX_HEIGHT);
  GRect finish_frame =  GRect(0, (pixels_to_scroll_by < 0 ? len : 0), WINDOW_WIDTH, TEXTBOX_HEIGHT);
  
  s_box_animation = property_animation_create_layer_frame(text_layer_get_layer(s_output_layer), &start_frame, &finish_frame);
  animation_set_handlers((Animation*)s_box_animation, (AnimationHandlers) {
    .stopped = anim_stopped_handler
  }, NULL);
  animation_set_duration((Animation*)s_box_animation, abs(pixels_to_scroll_by) * 35); // delay is proportional to text size
  animation_set_curve((Animation*)s_box_animation, AnimationCurveLinear);  // setting equal speed animation
  animation_set_delay((Animation*)s_box_animation, 3000); //initial delay of 3 seconds to let user start reading quote

  animation_schedule((Animation*)s_box_animation);
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

void turnOnAccelCollection()
  {
  // subscribe to the accel data 
  uint32_t num_samples = 25;
  accel_data_service_subscribe(num_samples, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
}

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
          accel_data_service_unsubscribe();
        }
        if(strcmp(t->value->cstring, "TURN ON") == 0)
        {
          uint32_t num_samples = 25;
          accel_data_service_subscribe(num_samples, data_handler);
          accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
        }
        break;
      case 2:
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY DATA: %d", (int)t->key);
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %s", t->value->cstring);
        vibes_short_pulse();
        layer_set_hidden((Layer *)static_messages, true);
        //text_layer_set_text(s_output_layer, "Hello my name is Rafael Aguayo. Hello my name is Rafael Aguayo. Hello my name is Rafael Aguayo. Hello my name is Rafael Aguayo.");
        text_layer_set_text(s_output_layer, t->value->cstring);
        // if height of question > height of window, initiate animation to scroll
        GSize text_size = text_layer_get_content_size(s_output_layer);
        APP_LOG(APP_LOG_LEVEL_INFO, "Content size: %d", text_size.h);
        number_of_pixels = WINDOW_HEIGHT - text_size.h;
        APP_LOG(APP_LOG_LEVEL_INFO, "Number of pixels: %d", number_of_pixels);

        if (number_of_pixels <= 0) {
            animate_question(number_of_pixels);
        }
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

// Called when an outgoing message from watch app was sent successfully
static void out_sent_handler(DictionaryIterator *sent, void *context) {
    // outgoing message was delivered
    APP_LOG(APP_LOG_LEVEL_DEBUG, "DICTIONARY SENT SUCCESSFULLY!");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Size of Dict: %d", (int)dict_size(sent));
}

// Called when an incoming message from iPhone is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message was dropped
     APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
}

// Called when an outgoing message from watch app is dropped
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    // outgoing message failed
    APP_LOG(APP_LOG_LEVEL_DEBUG, "DICTIONARY NOT SENT! ERROR!");
   //APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
}

// when message is recieved, click up button to confirm activity
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  animation_unschedule_all();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "UP Button Pressed!");
  layer_set_hidden((Layer *)s_output_layer, true);
  layer_set_hidden((Layer *)static_messages, false);
  text_layer_set_text(static_messages, "Confirmed as yes! Waiting for new message...");
  send_message("YES");
}

// when message is recieved, click down button to confirm it was not activity
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  animation_unschedule_all();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "DOWN Button Pressed!");  
  layer_set_hidden((Layer *)s_output_layer, true);
  layer_set_hidden((Layer *)static_messages, false);
  text_layer_set_text(static_messages, "Not now! Waiting for new message...");
  send_message("NO");
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(0, 0, WINDOW_WIDTH, TEXTBOX_HEIGHT));
  static_messages = text_layer_create(GRect(5, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(static_messages, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(static_messages, "No message recieved yet.");
  
  // add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
  layer_add_child(window_layer, text_layer_get_layer(static_messages));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
  text_layer_destroy(static_messages);
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