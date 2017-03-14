/*
 * This file is part of the CN24 semantic segmentation software,
 * copyright (C) 2015 Clemens-Alexander Brust (ikosa dot de at gmail dot com).
 *
 * For licensing information, see the LICENSE file included with this project.
 */

#include "ShellState.h"
#include <iomanip>
namespace Conv {
  
CN24_SHELL_FUNC_IMPL(DataList) {
  CN24_SHELL_FUNC_DESCRIPTION("Lists all currently loaded data");
  CN24_SHELL_PARSE_ARGS;
  
  std::cout << std::endl;
  std::cout << std::setw(10) << "Area" << std::setw(30) << "Bundle";
  std::cout << std::setw(30) << "Segment" << std::setw(10) << "Samples" <<
  std::endl;
  
  // List training area
  std::cout << std::setw(10) << "Training" << std::endl;
  for(unsigned int b = 0; b < training_bundles_->size(); b++) {
    Bundle* bundle = training_bundles_->at(b);
    std::cout << std::setw(10) << "|" << std::setfill('.') << std::setw(30) << bundle->name << std::setfill(' ');
    std::cout << std::setw(40) << bundle->GetSampleCount() << std::endl;
    std::cout << std::setw(10) << "|" << std::setfill('.') << std::setw(25) << "Weight:" << std::setfill(' ') << std::setw(5) << training_weights_->at(b) << std::endl;
    for(unsigned int s = 0; s < bundle->GetSegmentCount(); s++) {
      Segment* segment = bundle->GetSegment(s); 
      if(b == training_bundles_->size() - 1) {
        std::cout << std::setw(40);
      } else {
        std::cout << std::setw(10) << "|" << std::setw(30);
      }
      std::cout << "|" << std::setfill('.') << std::setw(30) << segment->name << std::setfill(' ');
      std::cout << std::setw(10) << segment->GetSampleCount() << std::endl;
    }
  }
  std::cout << std::endl;
  
  // List staging and testing areas
  std::cout << std::setw(10) << "Staging" << std::endl;
  for(unsigned int b = 0; b < staging_bundles_->size(); b++) {
    Bundle* bundle = staging_bundles_->at(b);
    std::cout << std::setw(10) << "|" << std::setfill('.') << std::setw(30) << bundle->name << std::setfill(' ');
    std::cout << std::setw(40) << bundle->GetSampleCount() << std::endl;
    for(unsigned int s = 0; s < bundle->GetSegmentCount(); s++) {
      Segment* segment = bundle->GetSegment(s); 
      if(b == staging_bundles_->size() - 1) {
        std::cout << std::setw(40);
      } else {
        std::cout << std::setw(10) << "|" << std::setw(30);
      }
      std::cout << "|" << std::setfill('.') << std::setw(30) << segment->name << std::setfill(' ');
      std::cout << std::setw(10) << segment->GetSampleCount() << std::endl;
    }
  }
  std::cout << std::endl;
  
  std::cout << std::setw(10) << "Testing" << std::endl;
  for(unsigned int b = 0; b < testing_bundles_->size(); b++) {
    Bundle* bundle = testing_bundles_->at(b);
    std::cout << std::setw(10) << "|" << std::setfill('.') << std::setw(30) << bundle->name << std::setfill(' ');
    std::cout << std::setw(40) << bundle->GetSampleCount() << std::endl;
    for(unsigned int s = 0; s < bundle->GetSegmentCount(); s++) {
      Segment* segment = bundle->GetSegment(s); 
      if(b == testing_bundles_->size() - 1) {
        std::cout << std::setw(40);
      } else {
        std::cout << std::setw(10) << "|" << std::setw(30);
      }
      std::cout << "|" << std::setfill('.') << std::setw(30) << segment->name << std::setfill(' ');
      std::cout << std::setw(10) << segment->GetSampleCount() << std::endl;
    }
  }
  std::cout << std::endl;
  return SUCCESS;
}

CN24_SHELL_FUNC_IMPL(BundleLoad) {
  CN24_SHELL_FUNC_DESCRIPTION("Loads a JSON bundle into the staging area");
  char* file = nullptr;
  char* folder_hint = nullptr;
  cargo_add_option(cargo, (cargo_option_flags_t)0, "file", "JSON bundle file",
                   "s", &file);
  cargo_add_option(cargo, (cargo_option_flags_t)0, "--folder-hint",
    "Additional search folder for image data", "s", &folder_hint);
  CN24_SHELL_PARSE_ARGS;
  
  std::string path = PathFinder::FindPath(std::string(file), {});
  std::ifstream json_file(path, std::ios::in);
  if(path.length() == 0 || !json_file.good()) {
    LOGERROR << "Cannot open file: " << file;
    return FAILURE;
  }
  
  Bundle* bundle = new Bundle("New Import");
  try {
    JSON descriptor = JSON::parse(json_file);
    if(folder_hint != nullptr) {
      bundle->Deserialize(descriptor, std::string(folder_hint));
    } else {
      bundle->Deserialize(descriptor);
    }
  } catch (std::exception ex) {
    LOGERROR << "Could not load " << file << ":";
    LOGERROR << ex.what();
    delete bundle;
    return FAILURE;
  }
  
  staging_bundles_->push_back(bundle);
  return SUCCESS;
}

CN24_SHELL_FUNC_IMPL(BundleMove) {
  CN24_SHELL_FUNC_DESCRIPTION("Moves a bundle from one area to another");
  char* name = nullptr;
  char* target_area = nullptr;
  cargo_add_option(cargo, (cargo_option_flags_t)0, "bundle", "Name of the bundle to move",
    "s", &name);
  cargo_add_option(cargo, (cargo_option_flags_t)0, "target-area", "Name of the target area",
    "s", &target_area);
  cargo_add_validation(cargo, (cargo_validation_flags_t)0, "target-area",
    cargo_validate_choices((cargo_validate_choices_flags_t)
    CARGO_VALIDATE_CHOICES_CASE_SENSITIVE, CARGO_STRING,
    3, "training", "staging", "testing"));
  CN24_SHELL_PARSE_ARGS;
  
  std::string name_str(name);
  Bundle* bundle = DataTakeBundle(name_str);
  
  if(bundle == nullptr) {
    LOGERROR << "Could not find bundle \"" << name_str << "\".";
    return WRONG_PARAMS;
  }
  
  std::string target_area_str(target_area);
  if(target_area_str.compare("training") == 0) {
    training_bundles_->push_back(bundle);
    training_weights_->push_back(1);
  } else if(target_area_str.compare("staging") == 0) {
    staging_bundles_->push_back(bundle);
  } else if(target_area_str.compare("testing") == 0) {
    testing_bundles_->push_back(bundle);
  }

  DataUpdated();

  return SUCCESS;
}

CN24_SHELL_FUNC_IMPL(SegmentMove) {
  CN24_SHELL_FUNC_DESCRIPTION("Moves a segment from one bundle to another");

  char* segment_name = nullptr;
  char* source_bundle = nullptr;
  char* target_bundle = nullptr;

  cargo_add_option(cargo, (cargo_option_flags_t)0, "segment", "Name of the segment to move",
    "s", &segment_name);
  cargo_add_option(cargo, (cargo_option_flags_t)0, "source-bundle", "Name of the source bundle",
    "s", &source_bundle);
  cargo_add_option(cargo, (cargo_option_flags_t)0, "target-bundle", "Name of the target bundle",
    "s", &target_bundle);

  CN24_SHELL_PARSE_ARGS;

  std::string segment_name_str(segment_name);
  std::string source_bundle_str(source_bundle);
  std::string target_bundle_str(target_bundle);

  // First, check if target bundle exists
  Bundle* bundle = DataFindBundle(target_bundle);

  if(bundle == nullptr) {
    LOGERROR << "Could not find bundle \"" << target_bundle_str << "\".";
    return WRONG_PARAMS;
  }

  // Try to take segment from source bundle
  Segment* segment = DataTakeSegment(source_bundle_str, segment_name_str);

  if(segment == nullptr) {
    LOGERROR << "Could not find segment \"" << segment_name_str << "\"" << " in bundle \"" <<
      source_bundle_str << "\".";
    return WRONG_PARAMS;
  }


  // Put segment in target bundle
  bundle->AddSegment(segment);

  DataUpdated();
  return SUCCESS;
}

Bundle* ShellState::DataFindBundle(const std::string& name) {
  for(unsigned int b = 0; b < training_bundles_->size(); b++) {
    Bundle* bundle = training_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      return bundle;
    }
  }
  for(unsigned int b = 0; b < staging_bundles_->size(); b++) {
    Bundle* bundle = staging_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      return bundle;
    }
  }
  for(unsigned int b = 0; b < testing_bundles_->size(); b++) {
    Bundle* bundle = testing_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      return bundle;
    }
  }
  return nullptr;
}

Bundle* ShellState::DataTakeBundle(const std::string& name) {
  for(unsigned int b = 0; b < training_bundles_->size(); b++) {
    Bundle* bundle = training_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      training_bundles_->erase(training_bundles_->begin() + b);
      training_weights_->erase(training_weights_->begin() + b);
      return bundle;
    }
  }
  for(unsigned int b = 0; b < staging_bundles_->size(); b++) {
    Bundle* bundle = staging_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      staging_bundles_->erase(staging_bundles_->begin() + b);
      return bundle;
    }
  }
  for(unsigned int b = 0; b < testing_bundles_->size(); b++) {
    Bundle* bundle = testing_bundles_->at(b);
    if(bundle->name.compare(name) == 0) {
      testing_bundles_->erase(testing_bundles_->begin() + b);
      return bundle;
    }
  }
  return nullptr;
}

Segment* ShellState::DataFindSegment(const std::string &bundle_name, const std::string &segment_name) {
  // Find bundle
  Bundle* bundle = DataFindBundle(bundle_name);
  if(bundle == nullptr) {
    return nullptr;
  }

  int segment_index = bundle->GetSegmentIndex(segment_name);
  if(segment_index == -1) {
    return nullptr;
  }

  // Find segment
  Segment* segment = bundle->GetSegment(segment_index);

  return segment;
}

Segment* ShellState::DataTakeSegment(const std::string &bundle_name, const std::string &segment_name) {
  // Find bundle
  Bundle* bundle = DataFindBundle(bundle_name);
  if(bundle == nullptr) {
    return nullptr;
  }

  int segment_index = bundle->GetSegmentIndex(segment_name);
  if(segment_index == -1) {
    return nullptr;
  }

  // Find segment
  Segment* segment = bundle->GetSegment(segment_index);

  // Remove segment
  bundle->RemoveSegment(segment_index);

  return segment;
}

void ShellState::DataUpdated() {
  // Update input layer's datasets if possible
  switch(state_) {
    case NOTHING:
      break;
    case NET_AND_TRAINER_LOADED:
    case NET_LOADED:
      input_layer_->UpdateDatasets();
      break;

  }
}

}