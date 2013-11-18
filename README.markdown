Swipe Keyboards for iOS
=========

#### New iOS keyboard ####
Swipe keyboard proto

###  This project uses code from
Kulpreet Chilana
Apple, Inc.

### Usage
Add the `iPad` and/or `iPhone:iPod` directory to your target in Xcode. `PKCustomKeyboard.h` contains all of the customizable constants for the iPad keyboard. `PMCustomKeyboard.h` contains all of the customizable constants for the iPhone/iPod touch keyboard. Characters in the `kChar` array can start or end with ◌ (U+25CC) and the ◌ will not appear when inputted into the textView. This is particularly useful for diacritical characters.

Make sure to import `PKCustomKeyboard.h` and `PMCustomKeyboard.h` into your view controller's header file.

Insert the following into the `viewDidLoad` of your view controller or wherever else you see fit. You can `setTextView` to any `UITextView` or `UITextField` that you want the keyboard to appear for.

```objective-c

if ( UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone )
    {
        PMCustomKeyboard *customKeyboard = [[PMCustomKeyboard alloc] init];
        [customKeyboard setTextView:self.textView];
        
    }
    else {
        PKCustomKeyboard *customKeyboard = [[PKCustomKeyboard alloc] init];
        [customKeyboard setTextView:self.textView];
    }

```
