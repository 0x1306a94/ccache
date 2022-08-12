//
//  ViewController.m
//  ccache-sample
//
//  Created by king on 2022/8/2.
//

#import "ViewController.h"

#import <IGListDiffKit/IGListDiffKit.h>
#import <IGListKit/IGListKit.h>
#import <Masonry/Masonry.h>

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.

#define MAS_MAKE(view, ...)                                                         \
    do {                                                                            \
        view.translatesAutoresizingMaskIntoConstraints = NO;                        \
        MASConstraintMaker *maker = [[MASConstraintMaker alloc] initWithView:view]; \
        __VA_ARGS__;                                                                \
        [maker install];                                                            \
    } while (0)

#define MAS_UPDATE(view, ...)                                                       \
    do {                                                                            \
        view.translatesAutoresizingMaskIntoConstraints = NO;                        \
        MASConstraintMaker *maker = [[MASConstraintMaker alloc] initWithView:view]; \
        maker.updateExisting = YES;                                                 \
        __VA_ARGS__;                                                                \
        [maker install];                                                            \
    } while (0)

#define MAS_REMAKE(view, ...)                                                       \
    do {                                                                            \
        view.translatesAutoresizingMaskIntoConstraints = NO;                        \
        MASConstraintMaker *maker = [[MASConstraintMaker alloc] initWithView:view]; \
        maker.removeExisting = YES;                                                 \
        __VA_ARGS__;                                                                \
        [maker install];                                                            \
    } while (0)

    UIView *view = [[UIView alloc] init];
    view.backgroundColor = UIColor.orangeColor;
    [self.view addSubview:view];
    MAS_MAKE(view, {
        maker.centerX.mas_equalTo(self.view.mas_centerX);
        maker.centerY.mas_equalTo(self.view.mas_centerY);
        maker.size.mas_equalTo(CGSizeMake(100, 100));
    });

    MAS_UPDATE(view, {
        maker.size.mas_equalTo(CGSizeMake(20, 60));
    });
    
//
//    MAS_REMAKE(view, {
//        maker.size.mas_equalTo(CGSizeMake(20, 60));
//    });
}

@end

