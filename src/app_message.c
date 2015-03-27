#include <pebble.h>

static Window *window;	
static TextLayer *s_output_layer;	
static int counter = 0;

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 42
};

// // Write message to buffer & send
// void send_message(cstring str){
// 	DictionaryIterator *iter;
	
// 	app_message_outbox_begin(&iter);
// 	dict_write_cstring(iter, MESSAGE_KEY, "YES");
	
// 	dict_write_end(iter);
//   	app_message_outbox_send();
// }

// data handler to recieve accel data from watch
static void data_handler(AccelData *data, uint32_t num_samples) {
  // Long lived buffer
  static char s_buffer[128];
  counter++;
  if(counter <= 20) {
  
  // Compose string of all data for 3 samples
  snprintf(s_buffer, sizeof(s_buffer), 
    "N X,Y,Z\n0 %d,%d,%d\n1 %d,%d,%d\n2 %d,%d,%d\n3 %d", 
    data[0].x, data[0].y, data[0].z, 
    data[1].x, data[1].y, data[1].z, 
    data[2].x, data[2].y, data[2].z, counter
  );
    
  }
  
  if(counter == 60) {
    counter = 0;
   uint32_t timeout_ms = 20000;
   app_timer_register(timeout_ms, turn_off_accel_collection, NULL);
  }

  //Show the data
  text_layer_set_text(s_output_layer, s_buffer);
}


static void turn_off_accel_collection(void* data) {
    accel_data_service_unsubscribe();
    uint32_t timeout_ms = 40000;
    app_timer_register(timeout_ms, turn_on_accel_collection, NULL);

}

static void turn_on_accel_collection(void* data) {
    // subscribe to the accel data 
    uint32_t num_samples = 25;
    accel_data_service_subscribe(num_samples, data_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
}

// Called when a message is received from iPhone
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
	
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
      text_layer_set_text(s_output_layer, tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
}

// when message is recieved, click up button to confirm activity
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_output_layer, "Confirmed as yes!");
  
}

// when message is recieved, click down button to confirm it was not activity
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_output_layer, "Confirmed as no!");
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// Called when an incoming message from iPhone is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when iPhone does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
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
	app_message_register_outbox_failed(out_failed_handler);
  
  // subscribe to the accel data 
  uint32_t num_samples = 25;
  accel_data_service_subscribe(num_samples, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  
  //subscribe to a timer
  uint32_t timeout_ms = 20000;
  app_timer_register(timeout_ms, turn_off_accel_collection, NULL);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	}

void deinit(void) {
	app_message_deregister_callbacks();
  accel_data_service_unsubscribe();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}