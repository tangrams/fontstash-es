//
//  DetailViewController.h
//  FontstashiOS
//
//  Created by Karim Naaji on 07/10/2014.
//  Copyright (c) 2014 Karim Naaji. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DetailViewController : UIViewController

@property (strong, nonatomic) id detailItem;
@property (weak, nonatomic) IBOutlet UILabel *detailDescriptionLabel;

@end

