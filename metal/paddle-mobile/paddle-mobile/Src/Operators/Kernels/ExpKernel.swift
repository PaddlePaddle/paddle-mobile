/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License. */

import Foundation

class ExpKernel<P: PrecisionProtocol>: Kernel, Computable {
    
    func compute(commandBuffer: MTLCommandBuffer, param: ExpParam<P>) throws {
        guard let encoder = commandBuffer.makeComputeCommandEncoder() else {
            let error = PaddleMobileError.predictError(message: "encoder is nil")
            throw paddleMobileLogAndThrow(error: error)
        }
        guard let tempPipline = pipline else {
            let error = PaddleMobileError.predictError(message: "pipline is nil")
            throw paddleMobileLogAndThrow(error: error)
        }
        encoder.setTexture(param.input.metalTexture, index: 0)
        encoder.setTexture(param.output.metalTexture, index: 1)
        encoder.dispatch(computePipline: tempPipline, outTexture: param.output.metalTexture)
        encoder.endEncoding()
    }
    
    required init(device: MTLDevice, param: ExpParam<P>, initContext: InitContext) throws {
        try param.output.initTexture(device: device, inTranspose: param.input.transpose, computePrecision: GlobalConfig.shared.computePrecision)
        if GlobalConfig.shared.computePrecision == .Float32 {
            try super.init(device: device, inFunctionName: "exp", initContext: initContext)
        } else if GlobalConfig.shared.computePrecision == .Float16 {
            try super.init(device: device, inFunctionName: "exp_half", initContext: initContext)
        } else {
            let error = PaddleMobileError.predictError(message: "unsupported compute precision: \(GlobalConfig.shared.computePrecision)")
            throw paddleMobileLogAndThrow(error: error)
        }
    }
}
